#pragma once

#include <string>
#include <map>
#include <deque>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <future>
#include <stdexcept>

namespace gcm {
namespace pubsub {

using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using Duration = std::chrono::seconds;

static constexpr Duration DefaultTimeout = std::chrono::seconds(60);

class Exception: public std::runtime_error {
public:
    Exception(const char *message): std::runtime_error(message)
    {}
};

class SessionNotFound: public Exception {
public:
    SessionNotFound(): Exception("Requested session was not found.")
    {}
};

template<typename T>
constexpr
std::enable_if_t<std::is_scalar<T>::value, T> DefaultValue() {
    return 0;
}

template<typename T>
constexpr
std::enable_if_t<!std::is_scalar<T>::value, T> DefaultValue() {
    return T();
}

template<typename T>
class IdSource {
public:
    using IdType = T;

    // Won't work for scalar types.
    IdSource(): last_id(DefaultValue<T>())
    {}

    IdSource(const IdSource &) = delete;
    IdSource(IdSource &&) = default;

    T get() {
        ++last_id;
        return last_id;
    }

protected:
    T last_id;
};

struct SessionInfo {
    TimePoint last_activity;
    Duration timeout;
    std::size_t num_messages;
};

template<typename ValueType>
class Session {
public:
    template<typename T>
    Session(T timeout):
        last_activity(Clock::now()),
        timeout(std::chrono::duration_cast<Duration>(timeout))
    {}

    Session(const Session &) = delete;
    Session(Session &&) = default;

    void publish(const ValueType &value) {
        std::unique_lock<std::mutex> lock(mutex);
        messages.push_back(value);
        cv.notify_all();
    }

    template<typename T>
    bool wait_for(T duration) {
        std::unique_lock<std::mutex> lock(mutex);
        return cv.wait_for(lock, duration, [&](){
            return !messages.empty();
        });
    }

    template<typename T>
    std::vector<ValueType> poll(T timeout) {
        reset_activity();

        if (timeout.count() > 0) {
            wait_for(timeout);
        }

        return poll();
    }

    std::vector<ValueType> poll() {
        reset_activity();
        std::vector<ValueType> out;

        std::unique_lock<std::mutex> lock(mutex);
        for (auto &v: messages) {
            out.push_back(v);
        }
        messages.clear();

        return std::move(out);
    }

    bool is_alive(const TimePoint &now) {
        return (last_activity + timeout) > now;
    }

    SessionInfo get_info() {
        return {
            last_activity,
            timeout,
            messages.size()
        };
    }

protected:
    void reset_activity() {
        last_activity = Clock::now();
    }

    std::mutex mutex;
    TimePoint last_activity;
    Duration timeout;
    std::deque<ValueType> messages;
    std::condition_variable cv;
};

template<typename ValueType>
class Channel {
public:
    using IdSource = gcm::pubsub::IdSource<uint64_t>;

    Channel() = default;
    Channel(const Channel &) = default;
    Channel(Channel &&) = delete;

    /**
     * Publish message to all sessions on this channel.
     */
    void publish(const ValueType &value) {
        reset_activity();
        std::unique_lock<std::mutex> channel_lock(mutex);

        for (auto &it: sessions) {
            it.second->publish(value);
        }
    }

    /**
     * Subscribe new session to this channel with custom session timeout.
     */
    template<typename T>
    IdSource::IdType subscribe(T timeout) {
        reset_activity();
        std::unique_lock<std::mutex> channel_lock(mutex);

        IdSource::IdType new_id = id_source.get();
        sessions.emplace(std::make_pair(
            new_id,
            std::make_shared<Session<ValueType>>(timeout)
        ));

        return new_id;
    }

    /**
     * Subscribe new session to this channel with default timeout.
     */
    IdSource::IdType subscribe() {
        reset_activity();
        std::unique_lock<std::mutex> channel_lock(mutex);

        IdSource::IdType new_id = id_source.get();
        sessions.emplace(std::make_pair(
            new_id,
            std::make_shared<Session<ValueType>>(DefaultTimeout)
        ));

        return new_id;   
    }

    /**
     * Non timed query for new messages in session.
     */
    std::vector<ValueType> poll(const IdSource::IdType &session) {
        return get_session(session)->poll();
    }

    /**
     * Timed wait for new messages in session.
     */
    template<typename T>
    std::vector<ValueType> poll(const IdSource::IdType &session, T duration) {
        return get_session(session)->poll(duration);
    }

    /**
     * List sessions active on this channel.
     */
    std::vector<IdSource::IdType> list_sessions() {
        std::vector<IdSource::IdType> out;

        for (auto &it: sessions) {
            out.push_back(it.first);
        }

        return out;
    }

    /**
     * Return information of one session.
     */
    SessionInfo get_session_info(const IdSource::IdType &session) {
        return get_session(session)->get_info();
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
                if (!it->second->is_alive(now)) {
                    sessions.erase(it);
                    changed = true;
                } else {
                    res = true;
                }
            }
        }

        return res || (last_activity - Clock::now()) > DefaultTimeout;
    }

protected:
    void reset_activity() {
        last_activity = Clock::now();
    }

    std::shared_ptr<Session<ValueType>> get_session(const IdSource::IdType &session) {
        reset_activity();

        std::unique_lock<std::mutex> channel_lock(mutex);

        auto it = sessions.find(session);
        if (it != sessions.end()) {
            return it->second;
        } else {
            throw SessionNotFound();
        }
    }

    std::map<IdSource::IdType, std::shared_ptr<Session<ValueType>>> sessions;
    std::mutex mutex;
    IdSource id_source;
    TimePoint last_activity;
};

template<typename ChannelNameType, typename ValueType>
class PubSub {
public:
    using IdType = typename Channel<ValueType>::IdSource::IdType;

    PubSub():
        quit(false),
        clean_task(std::async(std::launch::async, &PubSub::clean_thread, this))
    {}

    PubSub(const PubSub &) = delete;
    PubSub(PubSub &&) = default;

    ~PubSub() {
        quit = true;
        cv.notify_all();
        clean_task.get();
    }

    void publish(const ChannelNameType &channel, const ValueType &value) {
        get_channel(channel)->publish(value);
    }

    template<typename T>
    IdType subscribe(const ChannelNameType &channel, T timeout) {
        return get_channel(channel)->subscribe(timeout);
    }

    IdType subscribe(const ChannelNameType &channel) {
        return get_channel(channel)->subscribe();
    }

    std::vector<ValueType> poll(const ChannelNameType &channel, const IdType &session) {
        return get_channel(channel)->poll(session);
    }

    template<typename T>
    std::vector<ValueType> poll(const ChannelNameType &channel, const IdType &session, T duration) {
        return get_channel(channel)->poll(session, duration);
    }

    std::vector<ChannelNameType> list_channels() {
        std::vector<ChannelNameType> out;

        for (auto &it: channels) {
            out.push_back(it.first);
        }

        return out;
    }

    std::vector<IdType> list_sessions(const ChannelNameType &channel) {
        return get_channel(channel)->list_sessions();
    }

    SessionInfo get_session_info(const ChannelNameType &channel, const IdType &session) {
        return get_channel(channel)->get_session_info(session);
    }

protected:
    std::shared_ptr<Channel<ValueType>> get_channel(const ChannelNameType &channel) {
        std::unique_lock<std::mutex> lock(mutex);

        auto it = channels.find(channel);
        if (it == channels.end()) {
            auto it = channels.emplace(std::make_pair(
                channel,
                std::make_shared<Channel<ValueType>>()
            ));
            return it.first->second;
        } else {
            return it->second;
        }
    }

    bool clean_thread() {
        while (!quit) {
            std::unique_lock<std::mutex> lock(mutex);
            cv.wait_for(lock, std::chrono::seconds(1), [&](){ return quit; });

            // Do the cleanup of channels.
            bool changed = true;
            while (changed) {
                changed = false;

                for (auto it = channels.begin(); it != channels.end(); ++it) {
                    if (!it->second->test_active_sessions()) {
                        channels.erase(it);
                        changed = true;
                        break;
                    }
                }
            }
        }

        return true;
    }

    std::map<ChannelNameType, std::shared_ptr<Channel<ValueType>>> channels;

    bool quit;
    std::mutex mutex;
    std::condition_variable cv;
    std::future<bool> clean_task;
};

} // namespace pubsub
} // namespace gcm
