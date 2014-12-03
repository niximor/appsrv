/**
 * RTJS (Real-Time JavaScript) push event module for gcm::AppSrv.
 * @author Michal Kuchta <niximor@gmail.com>
 */

#include <functional>

#include <gcm/json/validator.h>
#include <gcm/appsrv/json_rpc_api.h>

#include "rtjs.h"

Rtjs::Rtjs(gcm::json::rpc::RpcApi &api):
    server(api),
    log(api.get_logger())
{
    using namespace gcm::json::validator;
    using namespace std::placeholders;

    server.register_method("rtjs.subscribe", std::bind(&Rtjs::subscribe, this, _1),
        "Subscribe to given publication channel.",
        ParamDefinitions(
            String("channelName", "Name of channel to subscribe to."),
            Optional(Int("timeout", "Subscription timeout, in seconds."), gcm::json::make_int(60))
        ),
        Object("response", "Response",
            Int("sessionId", "ID of RTJS session for this channel.")
        )
    );

    server.register_method("rtjs.unsubscribe", std::bind(&Rtjs::unsubscribe, this, _1),
        "Unsubscribe from given publication channel.",
        ParamDefinitions(
            String("channelName", "Name of channel to unsubscribe from."),
            Int("sessionId", "ID of subscribed session.")
        ),
        Object("response", "Response",
            Bool("success", "Success of operation."),
            Optional(String("message", "Optional message in case of error."))
        )
    );

    server.register_method("rtjs.list", std::bind(&Rtjs::list, this, _1),
        "List currently active channels.",
        ParamDefinitions(),
        Object("response", "Response",
            Array("channels", "List of active channels",
                Object("channel", "Channel information",
                    String("name", "Channel name"),
                    Array("sessions", "List of existing sessions",
                        Object("sessionInfo", "Info of session",
                            Int("id", "Session ID"),
                            Int("lastActivity", "Number of seconds since last activity"),
                            Int("timeout", "Timeout value"),
                            Int("numMessages", "Number of unpolled messages in queue")
                        )
                    )
                )
            )
        )
    );

    server.register_method("rtjs.publish", std::bind(&Rtjs::publish, this, _1),
        "Publish new message to specified channel.",
        ParamDefinitions(
            String("channelName", "Name of channel where to publish"),
            AnyType("message", "Message data to publish")
        ),
        Bool("success", "Success of operation")
    );

    server.register_method("rtjs.poll", std::bind(&Rtjs::list, this, _1),
        "Poll session in channel for new messages and return them. Optionally wait for specified "
        "amount of time for new message to arrive.",
        ParamDefinitions(
            String("channelName", "Name of channel"),
            Int("sessionId", "ID of session subscribed to that channel"),
            Optional(Int("timeout", "If specified, wait for specified amount of time for new messages, "
                "if there are no messages currently in queue."))
        ),
        Array("messages", "List of queued messages.",
            AnyType("message", "Queued message.")
        )
    );
}

gcm::json::JsonValue Rtjs::subscribe(gcm::json::Array &params) {
    using namespace gcm::json;

    auto channelName = to<String>(params[0]);
    auto timeout = to<Int>(params[1]);

    auto id = pubsub.subscribe(channelName, std::chrono::seconds{timeout});

    auto res = make_object();
    auto &obj = to<Object>(res);
    obj["sessionId"] = make_int(id);

    return res;
}

gcm::json::JsonValue Rtjs::list(gcm::json::Array &) {
    using namespace gcm::json;

    auto res = make_object();
    auto &obj = to<Object>(res);

    auto &arr = to<Array>(obj["channels"] = make_array());

    for (auto &channel_name: pubsub.list_channels()) {
        auto p_channel = make_object();
        auto &channel = to<Object>(p_channel);

        channel["name"] = make_string(channel_name);

        auto &sessions = to<Array>(channel["sessions"] = make_array());
        for (auto &session_id: pubsub.list_sessions(channel_name)) {
            auto p_session = make_object();
            auto &session = to<Object>(p_session);

            gcm::pubsub::SessionInfo info = pubsub.get_session_info(channel_name, session_id);
            auto now = gcm::pubsub::Clock::now();
            auto activity_duration = now - info.last_activity;

            session["id"] = make_int(session_id);
            session["numMessages"] = make_int(info.num_messages);
            session["lastActivity"] = make_int(std::chrono::duration_cast<std::chrono::seconds>(activity_duration).count());
            session["timeout"] = make_int(info.timeout.count());

            sessions.push_back(p_session);
        }

        arr.push_back(p_channel);
    }

    return res;
}

gcm::json::JsonValue Rtjs::unsubscribe(gcm::json::Array &) {
    using namespace gcm::json;

    auto res = make_object();
    auto &obj = to<Object>(res);

    obj["success"] = make_bool(false);
    obj["message"] = make_string("Unsubscribe is not supported so far.");

    return res;
}

gcm::json::JsonValue Rtjs::publish(gcm::json::Array &) {
    using namespace gcm::json;

    return make_bool(true);
}

gcm::json::JsonValue Rtjs::poll(gcm::json::Array &) {
    using namespace gcm::json;

    return make_array();
}