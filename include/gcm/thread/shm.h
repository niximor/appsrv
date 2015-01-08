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
 * @date 2014-10-28
 *
 */

#pragma once

#include <chrono>

#include <sys/shm.h>
#include <errno.h>
#include <pthread.h>

#include <gcm/io/exception.h>

namespace gcm {
namespace thread {

template<typename T>
class SharedMemory {
public:
    template<typename... Params>
    SharedMemory(Params... attrs):
        shmptr(nullptr),
        shmid(0),
        creator_pid(getpid())
    {
        shmid = shmget(IPC_PRIVATE, sizeof(T), IPC_CREAT | 0600);
        if (shmid < 0) {
            throw gcm::io::IOException(errno);
        }

        void *ptr = shmat(shmid, NULL, 0);
        if (ptr == (void *)-1) {
            throw gcm::io::IOException(errno);
        }

        shmptr = (T *)ptr;

        // Call constructor of the object in allocated shared memory segment.
        new (shmptr) T(attrs...);
    }

    SharedMemory(const SharedMemory &) = delete;

    SharedMemory(SharedMemory &&other):
        shmptr(other.shmptr),
        shmid(other.shmid),
        creator_pid(other.creator_pid)
    {
        other.shmptr = nullptr;
        other.shmid = 0;
    }

    ~SharedMemory() {
        // Only in creator process...
        if (should_destroy()) {
            // Call destructor of the object in shared memory segment.
            shmptr->~T();
        }

        if (shmptr != nullptr) {
            shmdt((void *)shmptr);
        }

        // Free memory only in parent process.
        if (creator_pid == getpid() && shmid > 0) {
            // Free the shared memory.
            shmctl(shmid, IPC_RMID, NULL);
        }
    }

    T &operator *() {
        return *shmptr;
    }

    T *operator ->() {
        return shmptr;
    }

protected:
    T *shmptr;
    int shmid;
    pid_t creator_pid;

    bool should_destroy() {
        return creator_pid == getpid() && shmid > 0 && shmptr != nullptr;
    }
};

/**
 * Mutex stored in shared memory, for inter-process synchronization.
 */
class SharedMutex: public SharedMemory<pthread_mutex_t> {
public:
    using native_handle_type = pthread_mutex_t *;

    SharedMutex(): SharedMemory<pthread_mutex_t>()
    {
        if (shmptr != nullptr) {
            pthread_mutexattr_init(&attrmutex);
            pthread_mutexattr_setpshared(&attrmutex, PTHREAD_PROCESS_SHARED);

            pthread_mutex_init(shmptr, &attrmutex);
        }
    }

    ~SharedMutex() {
        if (should_destroy()) {
            pthread_mutex_destroy(shmptr);
        }

        pthread_mutexattr_destroy(&attrmutex);
    }

    void lock() {
        int res = pthread_mutex_lock(shmptr);
        if (res != 0) {
            throw gcm::io::IOException(res);
        }
    }

    void unlock() {
        int res = pthread_mutex_unlock(shmptr);
        if (res != 0) {
            throw gcm::io::IOException(res);
        }
    }

    bool try_lock() {
        int res = pthread_mutex_trylock(shmptr);
        if (res == 0) {
            return true;
        } else if (res == EBUSY) {
            return false;
        } else {
            throw gcm::io::IOException(res);
        }
    }

    native_handle_type native_handle() {
        return shmptr;
    }

protected:
    pthread_mutexattr_t attrmutex;
};

/**
 * Condition variable stored in shared memory, for inter-process synchronization.
 */
class SharedCondition: public SharedMemory<pthread_cond_t> {
public:
    using native_handle_type = pthread_cond_t *;

    SharedCondition(): SharedMemory<pthread_cond_t>()
    {
        if (shmptr != nullptr) {
            pthread_condattr_init(&attrcond);
            pthread_condattr_setpshared(&attrcond, PTHREAD_PROCESS_SHARED);

            pthread_cond_init(shmptr, &attrcond);
        }
    }

    ~SharedCondition() {
        if (should_destroy()) {
            pthread_cond_destroy(shmptr);
        }

        pthread_condattr_destroy(&attrcond);
    }

    void notify_one() noexcept {
        pthread_cond_signal(shmptr);
    }

    void notify_all() noexcept {
        pthread_cond_broadcast(shmptr);
    }

    void wait(std::unique_lock<SharedMutex> &lock) {
        pthread_cond_wait(shmptr, lock.mutex()->native_handle());
    }

    template<typename Predicate>
    void wait(std::unique_lock<SharedMutex> &lock, Predicate pred) {
        while (!pred()) {
            wait(lock);
        }
    }

    template<typename Rep, typename Period>
    std::cv_status wait_for(std::unique_lock<SharedMutex> &lock, std::chrono::duration<Rep, Period> &rel_time) {
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(rel_time);
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(rel_time - seconds);

        timespec tmout;
        tmout.tv_sec = seconds.count();
        tmout.tv_nsec = ns.count();

        int res = pthread_cond_timedwait(shmptr, lock.mutex()->native_handle(), &tmout);
        if (res == 0) {
            return std::cv_status::no_timeout;
        } else if (res == ETIMEDOUT) {
            return std::cv_status::timeout;
        } else {
            throw gcm::io::IOException(res);
        }
    }

    template<typename Rep, typename Period, typename Predicate>
    bool wait_for(std::unique_lock<SharedMutex> &lock, std::chrono::duration<Rep, Period> &rel_time, Predicate pred) {
        while (!pred()) {
            if (wait_for(lock, rel_time) == std::cv_status::timeout) {
                return false;
            }
        }
        return true;
    }

    template<typename Clock, typename Duration>
    std::cv_status wait_until(std::unique_lock<SharedMutex> &lock, std::chrono::time_point<Clock, Duration> &timeout_time) {
        Duration dur = timeout_time - Clock::now();
        if (dur > Duration::zero()) {
            return wait_for(lock, dur);
        } else {
            return std::cv_status::timeout;
        }
    }

    template<typename Clock, typename Duration, typename Predicate>
    bool wait_until(std::unique_lock<SharedMutex> &lock, std::chrono::time_point<Clock, Duration> &timeout_time, Predicate pred) {
        while (!pred()) {
            if (wait_until(lock, timeout_time) == std::cv_status::timeout) {
                return false;
            }
        }
        return true;
    }

    native_handle_type native_handle() {
        return shmptr;
    }

protected:
    pthread_condattr_t attrcond;
};

}
}