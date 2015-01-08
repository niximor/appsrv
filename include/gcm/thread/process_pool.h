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

#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

#include <gcm/logging/logging.h>
#include <gcm/thread/shm.h>
#include <gcm/io/exception.h>

#include "state.h"

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
        pid = fork();

        if (pid < 0) {
            throw gcm::io::IOException(errno);
        } else if (pid == 0) {
            SignalBind bind{Signal::at(SIGINT, std::bind(&ProcessWorker::stop, this))};
            (*this)();
        } else if (pid > 0) {
            // Back in parent, do nothing here and return back
            // to pool.
        }
    }

    WorkerState get_state() {
        return state->state;
    }

    void operator()() {
        auto &log = gcm::logging::getLogger("GCM.ThreadPool");

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
                if (!pool.tasks.empty()) break;
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
        if (pid > 0) {
            // In parent
            kill(pid, SIGINT);

        } else if (pid == 0) {
            // In self.
            quit = true;
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
        keeper_thread(&ProcessPool::keeper_func, this),
        sigchld_bind(Signal::at(SIGCHLD, std::bind(&ProcessPool::sigchld, this)))
    {}

    ProcessPool(const ProcessPool &) = delete;
    ProcessPool(ProcessPool &&) = delete;

    void stop() {
        quit = true;
        keeper_cond.notify_all();

        std::unique_lock<SharedMutex> lock(worker_mutex);
        while (!workers.empty()) {
            auto it = workers.begin();
            (*it)->stop();
            workers.erase(it);
        }
        lock.unlock();
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

    SharedMemory<PoolState> state;

    bool quit;
    std::thread keeper_thread;

    std::condition_variable keeper_cond;
    std::mutex keeper_mutex;

    SignalBind sigchld_bind;

protected:
    void keeper_func() {
        while (!quit) {
            kill_spares();
            start_spares();

            std::unique_lock<std::mutex> lock(keeper_mutex);
            keeper_cond.wait_for(lock, std::chrono::milliseconds(check_interval));
        }
    }

    void kill_spares() {
        std::unique_lock<SharedMutex> lock(worker_mutex);

        int to_kill = (state->num_free + state->num_starting) - min_spare;

        if (to_kill > 0) {
            for (auto it = workers.begin(); to_kill > 0 && it != workers.end(); ++it) {
                if ((*it)->get_state() == WorkerState::Free) {
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
        std::unique_lock<SharedMutex> lock(worker_mutex);

        int to_start = min_spare - state->num_free - state->num_starting;
        if (workers.size() + to_start > max_workers) {
            to_start = max_workers - workers.size();
        }

        if (to_start > 0) {
            while (to_start > 0) {
                state->num_starting++;
                workers.emplace_back(std::make_unique<ProcessWorker<T>>(*this));
                --to_start;
            }
        }
    }

    void sigchld() {
        // Correctly terminate all killed processes.
        int status;
        while (waitpid(-1, &status, WNOHANG) > 0) {}
    }
};

}
}