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
            String("sessionId", "ID of RTJS session for this channel.")
        )
    );

    server.register_method("rtjs.unsubscribe", std::bind(&Rtjs::unsubscribe, this, _1),
        "Unsubscribe from given publication channel.",
        ParamDefinitions(
            String("channelName", "Name of channel to unsubscribe from."),
            String("sessionId", "ID of subscribed session.")
        ),
        Object("response", "Response",
            Int("status", "Status code of operation."),
            Optional(String("message", "Optional message in case of error."))
        )
    );
}

gcm::json::JsonValue Rtjs::subscribe(gcm::json::Array &params) {
    using namespace gcm::json;

    DEBUG(log) << "Params:";
    for (auto &param: params) {
        DEBUG(log) << param->to_string();
    }

    return make_object();
}

gcm::json::JsonValue Rtjs::unsubscribe(gcm::json::Array &) {
    using namespace gcm::json;

    return make_object();
}