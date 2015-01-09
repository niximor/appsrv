/**
 * Copyright 2014 Michal Kuchta <niximor@gmail.com>
 *
 * This file is part of GCM::AppSrv.
 *
 * GCM::AppSrv is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * GCM::AppSrv is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with GCM::AppSrv. If not, see http://www.gnu.org/licenses/.
 *
 * @author Michal Kuchta <niximor@gmail.com>
 * @date 2014-01-06
 *
 */

#pragma once

#include <deque>
#include <list>
#include <thread>

#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

#include <gcm/logging/logging.h>
#include <gcm/thread/shm.h>
#include <gcm/thread/signal.h>
#include <gcm/io/exception.h>

#include "state.h"

// Define this to see debug output from ProcessPool.
//#define PROCESSPOOL_DEBUG

namespace gcm {
namespace thread {

class ProcessState {
public:
    ProcessState(): state(WorkerState::Starting), jobs_processed(0)
    {}

    WorkerState state;
    int jobs_processed;
};

class PoolState {
public:
    int num_free;
    int num_starting;
    int num_busy;
};

template<typename T>
class ProcessPool;

template<typename T>
class ProcessWorker {
public:
    ProcessWorker(ProcessPool<T> &pool): pool(pool) {
        auto &log = gcm::logging::getLogger("GCM.ProcessPool");
#ifdef PROCESSPOOL_DEBUG
        DEBUG(log) << "Worker constructor.";
#endif

        pid = fork();

        if (pid < 0) {
            throw gcm::io::IOException(errno);
        } else if (pid == 0) {
#ifdef PROCESSPOOL_DEBUG
            DEBUG(log) << "Worker start in progress...";
#endif

            // Kill pool.
            pool.child_stop();

            setpgid(0, 0);
            SignalBind bind{Signal::at(SIGINT, std::bind(&ProcessWorker::stop, this))};
            (*this)();

            exit(0);
        } else if (pid > 0) {
            // Back in parent, do nothing here and return back
            // to pool.
#ifdef PROCESSPOOL_DEBUG
            DEBUG(log) << "Worker started.";
#endif
        }
    }

    WorkerState get_state() {
        return state->state;
    }

    pid_t get_pid() {
        if (pid > 0) {
            return pid;
        } else {
            return getpid();
        }
    }

    void operator()() {
        auto &log = gcm::logging::getLogger("GCM.ProcessPool");

        while (true) {
            if (state->state == WorkerState::Starting) {
                pool.state->num_free++;
                pool.state->num_starting--;
            } else {
                pool.state->num_free++;
            }

            state->state = WorkerState::Free;

            // Wait for work
            std::unique_lock<SharedMutex> lock(pool.worker_mutex);
            while (!quit) {
                // TODO: Wait for work
                //if (!pool.tasks.empty()) break;
                pool.worker_cond.wait(lock);
            }

            if (quit) {
                break;
            }

            state->state = WorkerState::Busy;
            pool.state->num_free--;
            pool.state->num_busy++;

            // TODO: Receive work
            T task;

            try {
                task();
            } catch (std::exception &e) {
                ERROR(log) << std::this_thread::get_id() << " Exception thrown while executing thread job: " << e.what();
            } catch (...) {
                ERROR(log) << std::this_thread::get_id() << " Unknown exception thrown while executing thread job.";
            }

            pool.state->num_busy--;
        }

        pool.state->num_free--;
    }

    void stop() {
        auto &log = gcm::logging::getLogger("GCM.ProcessPool");

        if (pid > 0) {
            // In parent
#ifdef PROCESSPOOL_DEBUG
            DEBUG(log) << "Stopping child " << pid << ".";
#endif
            kill(pid, SIGINT);

        } else if (pid == 0) {
            // In self.
#ifdef PROCESSPOOL_DEBUG
            DEBUG(log) << "Received SIGINT.";
#endif
            quit = true;
            pool.worker_cond.notify_all();
        }
    }

protected:
    ProcessPool<T> &pool;
    SharedMemory<ProcessState> state;
    pid_t pid;
    bool quit;
};

template<typename T>
class ProcessPool {
friend class ProcessWorker<T>;
public:
    ProcessPool(size_t min_spare = 5, size_t max_workers = 50, size_t check_interval = 100):
        min_spare(min_spare),
        max_workers(max_workers),
        check_interval(check_interval),
        quit(false),
        child_quit(false),
        keeper_thread(&ProcessPool::keeper_func, this),
        sigchld_bind(Signal::at(SIGCHLD, std::bind(&ProcessPool::sigchld, this)))
    {}

    ProcessPool(const ProcessPool &) = delete;
    ProcessPool(ProcessPool &&) = delete;

    void stop() {
        quit = true;
        keeper_cond.notify_all();
    }

    void add_work(T &&w) {
        std::unique_lock<SharedMutex> lock(worker_mutex);
        tasks.emplace_back(std::forward<T>(w));

        // TODO: Push work to one of processes.
    }

    ~ProcessPool() {
        stop();
        keeper_thread.join();
    }

protected:
    size_t min_spare;
    size_t max_workers;
    size_t check_interval;

    std::deque<T> tasks;

    SharedCondition worker_cond;
    SharedMutex worker_mutex;

    std::list<std::unique_ptr<ProcessWorker<T>>> workers;
    std::list<pid_t> to_collect;

    SharedMemory<PoolState> state;

    bool quit;
    bool child_quit;
    std::thread keeper_thread;

    std::condition_variable keeper_cond;
    std::mutex keeper_mutex;

    SignalBind sigchld_bind;

protected:
    void keeper_func() {
        auto &log = gcm::logging::getLogger("GCM.ProcessPool");

        while (!quit && !child_quit) {
            kill_spares();
            start_spares();

            std::unique_lock<std::mutex> lock(keeper_mutex);
            keeper_cond.wait_for(lock, std::chrono::milliseconds(check_interval));
        }

        if (child_quit) {
            return;
        }

#ifdef PROCESSPOOL_DEBUG
        DEBUG(log) << "ProcessPool stop in progress.";
#endif

        std::unique_lock<SharedMutex> lock(worker_mutex);
        while (!workers.empty()) {
            auto it = workers.begin();
            to_collect.push_back((*it)->get_pid());
            (*it)->stop();
            workers.erase(it);
        }
        lock.unlock();

        if (!to_collect.empty() || !workers.empty()) {
#ifdef PROCESSPOOL_DEBUG
            DEBUG(log) << "Waiting for child processes to terminate.";
#endif
        
            while (!to_collect.empty() || !workers.empty()) {
                std::unique_lock<std::mutex> lock(keeper_mutex);
                keeper_cond.wait_for(lock, std::chrono::milliseconds(check_interval));   
            }
        }
    }

    void kill_spares() {
        auto &log = gcm::logging::getLogger("GCM.ProcessPool");

        std::unique_lock<SharedMutex> lock(worker_mutex);

        int to_kill = (state->num_free + state->num_starting) - min_spare;

        if (to_kill > 0) {
#ifdef PROCESSPOOL_DEBUG
            DEBUG(log) << "Too many workers, killing " << to_kill << " of them.";
#endif
            for (auto it = workers.begin(); to_kill > 0 && it != workers.end(); ++it) {
                if ((*it)->get_state() == WorkerState::Free) {
                    to_collect.push_back((*it)->get_pid());
                    (*it)->stop();
                    workers.erase(it);

                    it = workers.begin();
                    --to_kill;
                }
            }

            // Notify all workers to allow them to quit.
            worker_cond.notify_all();
        }
    }

    void start_spares() {
        auto &log = gcm::logging::getLogger("GCM.ProcessPool");

        std::unique_lock<SharedMutex> lock(worker_mutex);

        int to_start = min_spare - state->num_free - state->num_starting;
        if (workers.size() + to_start > max_workers) {
            to_start = max_workers - workers.size();
        }

        pid_t pid = getpid();

        if (to_start > 0) {
#ifdef PROCESSPOOL_DEBUG
            DEBUG(log) << "Need to start " << to_start << " new workers.";
#endif
            while (to_start > 0) {
                state->num_starting++;
#ifdef PROCESSPOOL_DEBUG
                DEBUG(log) << "Starting new worker.";
#endif
                workers.emplace_back(std::make_unique<ProcessWorker<T>>(*this));
                --to_start;

                if (pid != getpid()) {
                    break;
                }
            }
        }
    }

    void sigchld() {
        // Correctly terminate all killed processes.
        int status;
        pid_t pid;
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            std::unique_lock<std::mutex> lock(keeper_mutex);
            // Try to find pid in to_collect.

            bool found = false;
            for (auto it = to_collect.begin(); it != to_collect.end(); ++it) {
                if ((*it) == pid) {
                    to_collect.erase(it);
                    found = true;
                    break;
                }
            }

            if (!found) {
                for (auto it = workers.begin(); it != workers.end(); ++it) {
                    if ((*it)->get_pid() == pid) {
                        workers.erase(it);
                        found = true;
                        break;
                    }
                }
            }

            if (!found) {
                auto &log = gcm::logging::getLogger("GCM.ProcessPool");
                WARNING(log) << "Received SIGCHLD for unknown PID " << pid << ".";
            }
        }
    }

    void child_stop() {
        std::unique_lock<SharedMutex> lock(worker_mutex);

        min_spare = 0;
        max_workers = 0;
        workers.clear();
        to_collect.clear();
        tasks.clear();
        child_quit = true;

        lock.unlock();

        stop();
    }
};

}
}