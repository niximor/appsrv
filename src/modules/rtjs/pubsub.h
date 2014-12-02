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

static constexpr Duration DefaultTimeout = std::chrono::seconds(60);

template<typename T>
class IdSource {
public:
    using IdType = T;

    // Won't work for scalar types.
    IdSource(): last_id(init_type())
    {}

    T get() {
        ++last_id;
        return last_id;
    }

protected:
    T last_id;

    static constexpr
    std::enable_if_t<std::is_scalar<T>::value, T> init_type() {
        return 0;
    }

    static constexpr
    std::enable_if_t<!std::is_scalar<T>::value, T> init_type() {
        return T();
    }
};


template<typename ValueType>
class Session {
public:
    friend class Channel<ValueType>;

    Session(Duration timeout = std::duration_cast<Duration>(DefaultTimeout)):
        last_activity(Clock::now()),
        timeout(timeout)
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
    using IdSource = gcm::pubsub::IdSource<uint64_t>;

    void publish(ValueType &&value) {
        std::unique_lock<std::mutex> channel_lock(mutex);

        for (auto &session: sessions) {
            session.publish(value);
        }
    }

    IdSource::IdType subscribe(std::chrono::seconds timeout) {
        IdSource::IdType new_id = id_source.get();
        sessions.emplace_back(std::make_pair(new_id, Session<ValueType>()));
        return new_id;
    }

    std::vector<ValueType> poll(const IdSource::IdType &session) {
        std::unique_lock<std::mutex> channel_lock(mutex);
        sessions[session].poll();
    }

    template<typename T>
    bool wait_for(const IdSource::IdType &session, T duration) {
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
    std::map<IdSource::IdType, Session<ValueType>> sessions;
    std::mutex mutex;
    IdSource id_source;
};

template<typename ChannelName, typename ValueType>
class PubSub {
public:
    using IdType = Channel<ValueType>::IdSource::IdType;

    PubSub():
        quit(false),
        clean_task{std::async(std::launch::async, &PubSub::clean_thread, this)}
    {}

    ~PubSub() {
        quit = true;
        cv.notify_all();
        clean_task.get();
    }

    void publish(const ChannelName &channel, ValueType &&value) {
        std::unique_lock<std::mutex> lock(mutex);
        channels[channel].publish(std::forward<ValueType>(value));
    }

    IdType subscribe(const ChannelName &channel, std::chrono::seconds timeout) {
        std::unique_lock<std::mutex> lock(mutex);
        return channels[channel].subscribe(timeout);
    }

    std::vector<ValueType> poll(const ChannelName &channel, const IdType &session) {
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

    std::map<ChannelName, Channel<ValueType>> channels;

    bool quit;
    std::mutex mutex;
    std::condition_variable<std::mutex> cv;
    std::future<bool> clean_task;
};

} // namespace pubsub
} // namespace gcm
