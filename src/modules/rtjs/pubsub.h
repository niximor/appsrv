#pragma once

#include <string>
#include <map>
#include <deque>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <future>

namespace gcm {
namespace pubsub {

using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using Duration = std::chrono::seconds;

template<typename ValueType>
class Session {
public:
    friend class Channel<ValueType>;

    Session(Duration timeout = std::duration_cast<Duration>(std::chrono::seconds(60))): last_activity(Clock::now()), timeout(timeout)
    {}

    void publish(JsonValue &value) {
        std::unique_lock<std::mutex> lock(mutex);
        messages.push_back(value);
    }

    template<typename T>
    bool wait_for(T duration) {
        std::unique_lock<std::mutex> lock(mutex);
        return cv.wait_for(lock, duration, [&](){
            return messages.empty();
        });
    }

    std::vector<ValueType> poll() {
        std::vector<ValueType> out;

        std::unique_lock<std::mutex> lock(mutex);
        for (auto &v: messages) {
            out.push_back(v);
        }
        messages.clear();

        last_activity = Clock::now();

        return std::move(out);
    }

protected:
    std::mutex mutex;
    TimePoint last_activity;
    Duration timeout;
    std::deque<ValueType> messages;
    std::condition_variable<std::mutex> cv;
};

template<typename ValueType>
class Channel {
public:
    void publish(ValueType &value) {
        std::unique_lock<std::mutex> channel_lock(mutex);

        for (auto &session: sessions) {
            session.publish(value);
        }
    }

    std::string subscribe(std::chrono::seconds timeout) {
        
    }

    std::vector<ValueType> poll(const std::string &session) {
        std::unique_lock<std::mutex> channel_lock(mutex);
        sessions[session].poll();
    }

    template<typename T>
    bool wait_for(const std::string &session, T duration) {
        std::unique_lock<std::mutex> channel_lock(mutex);
        sessions[session].wait_for(duration);
    }

    /**
     * Do the cleanup of sessions and return true if there are any active.
     */
    bool test_active_sessions() {
        bool res = false;
        auto now = Clock::now();

        std::unique_lock<std::mutex> channel_lock(mutex);

        bool changed = true;
        while (changed) {
            changed = false;
            for (auto it = sessions.begin(); it != sessions.end(); ++it) {
                bool erase = false;

                {
                    std::unique_lock<std::mutex> lock(it->second.mutex);
                    if (it->second.last_activity + timeout < now) {
                        erase = true;
                        changed = true;
                    }
                }

                if (erase) {
                    sessions.erase(it);
                    break;
                }
            }
        }

        return res;
    }

protected:
    std::map<std::string, Session<ValueType>> sessions;
    std::mutex mutex;
};

template<typename ValueType>
class PubSub {
public:
    PubSub():
        quit(false),
        clean_task{std::async(std::launch::async, &PubSub::clean_thread, this)}
    {}

    ~PubSub() {
        quit = true;
        cv.notify_all();
        clean_task.get();
    }

    void publish(const std::string &channel, JsonValue value) {
        std::unique_lock<std::mutex> lock(mutex);
        channels[channel].publish(value);
    }

    std::string subscribe(const std::string &channel) {

    }

    std::vector<ValueType> poll(const std::string &channel, const std::string &session) {
        std::unique_lock<std::mutex> lock(mutex);
        return channels[channel].poll(session);
    }

protected:
    void clean_thread() {
        while (!quit) {
            std::unique_lock<std::mutex> lock(mutex);
            cv.wait_for(lock, std::chrono::seconds(1), [&](){ return quit; });

            // Do the cleanup of channels.
            bool changed = true;
            while (changed) {
                changed = false;

                for (auto it = channels.begin(); it != channels.end(); ++it) {
                    if (!channel.test_active_sessions()) {
                        channels.erase(it);
                        changed = true;
                        break;
                    }
                }
            }
        }
    }

    std::map<std::string, Channel<ValueType>> channels;

    bool quit;
    std::mutex mutex;
    std::condition_variable<std::mutex> cv;
    std::future<bool> clean_task;
};

} // namespace pubsub
} // namespace gcm
