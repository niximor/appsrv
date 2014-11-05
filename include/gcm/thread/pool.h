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
 * @date 2014-11-02
 *
 */

#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <deque>
#include <list>
#include <atomic>

#include <gcm/logging/logging.h>

namespace gcm {
namespace thread {

enum class WorkerState {
    Starting, Free, Busy
};

template<typename T>
class Pool;

template<typename T>
class Worker;

template<typename T>
class WorkerWrapper {
public:
    WorkerWrapper(Worker<T> *worker): worker(worker)
    {}

    WorkerWrapper(WorkerWrapper &&) = default;
    WorkerWrapper(const WorkerWrapper &) = default;

    void operator()() {
        (*worker)();
    }

protected:
    Worker<T> *worker;
};

template<typename T>
class Worker {
public:
    Worker(Pool<T> &pool):
        pool(pool),
        state(WorkerState::Starting),
        quit(false),
        thread(WorkerWrapper<T>(this))
    {
        auto &log = gcm::logging::getLogger("GCM.ThreadPool");
        DEBUG(log) << "Started new thread.";
    }

    Worker(const Worker<T> &) = delete;
    Worker(Worker<T> &&) = default;

    WorkerState get_state() {
        return state;
    }

    void operator()() {
        auto &log = gcm::logging::getLogger("GCM.ThreadPool");
        while (true) {
            state = WorkerState::Free;
            pool.num_free++;

            std::unique_lock<std::mutex> lock(pool.worker_mutex);
            while (pool.tasks.empty() && !quit) {
                DEBUG(log) << "Thread is going to sleep.";
                pool.worker_cond.wait(lock);
                DEBUG(log) << "Thread awaken.";
            }

            if (quit) {
                DEBUG(log) << "Thread quit request.";
                break;
            }

            DEBUG(log) << "Thread has work.";

            state = WorkerState::Busy;
            pool.num_free--;
            pool.num_busy++;

            T task{std::move(pool.tasks.front())};
            pool.tasks.pop_front();

            lock.unlock();

            try {
                task();
            } catch (std::exception &e) {
                ERROR(log) << "Exception thrown while executing thread job: " << e.what();
            } catch (...) {
                ERROR(log) << "Unknown exception thrown while executing thread job.";
            }

            DEBUG(log) << "Work has been done.";

            pool.num_busy--;
        }

        DEBUG(log) << "Thread finished.";
    }

    void stop() {
        quit = true;
    }

    std::thread &get_thread() {
        return thread;
    }

protected:
    Pool<T> &pool;
    WorkerState state;
    bool quit;
    std::thread thread;
};

template<typename T>
class Pool {
public:
    Pool(size_t min_spare = 5, size_t max_workers = 50, size_t check_interval = 100):
        min_spare(min_spare),
        max_workers(max_workers),
        check_interval(check_interval),
        num_free(0),
        num_busy(0),
        keeper_thread(&Pool::keeper_func, this),
        quit(false)
    {
        start_spares();
    }

    void stop() {
        kill_spares();

        quit = true;
        keeper_cond.notify_all();
    }

    void add_work(T &&w) {
        std::unique_lock<std::mutex> lock(worker_mutex);
        tasks.emplace_back(std::forward<T>(w));

        auto &log = gcm::logging::getLogger("GCM.ThreadPool");
        DEBUG(log) << "New work.";

        worker_cond.notify_one();

        DEBUG(log) << "Worker notified.";
    }

    ~Pool() {
        stop();
        keeper_thread.join();
    }

protected:
    friend class Worker<T>;

    size_t min_spare;
    size_t max_workers;
    size_t check_interval;

    std::deque<T> tasks;

    std::condition_variable worker_cond;
    std::mutex worker_mutex;

    std::list<Worker<T>> workers;
    std::list<Worker<T>> to_collect;

    std::atomic<int> num_free;
    std::atomic<int> num_busy;

    std::thread keeper_thread;
    bool quit;

    std::condition_variable keeper_cond;
    std::mutex keeper_mutex;

protected:
    void keeper_func() {
        while (!quit) {
            kill_spares();
            start_spares();

            while (!to_collect.empty()) {
                to_collect.back().get_thread().join();
                to_collect.pop_back();
            }

            std::unique_lock<std::mutex> lock(keeper_mutex);
            keeper_cond.wait_for(lock, std::chrono::milliseconds(check_interval));
        }
    }

    void kill_spares() {
        std::unique_lock<std::mutex> lock(worker_mutex);

        int to_kill = num_free - min_spare;

        if (to_kill > 0) {
            auto &log = gcm::logging::getLogger("GCM.ThreadPool");
            DEBUG(log) << "Stopping " << to_kill << " threads.";

            for (auto it = workers.begin(); to_kill > 0 && it != workers.end(); ++it) {
                if (it->get_state() == WorkerState::Free) {
                    it->stop();
                    to_collect.emplace_back(std::move(*it));
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
        std::unique_lock<std::mutex> lock(worker_mutex);

        int to_start = min_spare - num_free;
        if (workers.size() + to_start > max_workers) {
            to_start = max_workers - workers.size();
        }

        if (to_start > 0) {
            auto &log = gcm::logging::getLogger("GCM.ThreadPool");
            DEBUG(log) << "Starting " << to_start << " threads.";

            while (to_start > 0) {
                workers.emplace_back(*this);
                --to_start;
            }
        }
    }
};

}
}