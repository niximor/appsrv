/**
 * RTJS (Real-Time JavaScript) push event module for gcm::AppSrv.
 * @author Michal Kuchta <niximor@gmail.com>
 */

#pragma once

#include <gcm/appsrv/json_rpc_api.h>
#include <gcm/json/json.h>

class Rtjs {
public:
    Rtjs(gcm::json::rpc::RpcApi &api);

    gcm::json::JsonValue subscribe(gcm::json::Array &params);
    gcm::json::JsonValue unsubscribe(gcm::json::Array &params);

protected:
    gcm::json::rpc::RpcApi &server;
};
