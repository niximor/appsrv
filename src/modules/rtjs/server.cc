/**
 * RTJS (Real-Time JavaScript) push event module for gcm::AppSrv.
 * @author Michal Kuchta <niximor@gmail.com>
 */

#include <gcm/appsrv/json_rpc_api.h>

#include "rtjs.h"

class RtjsServer {
public:
    RtjsServer(gcm::json::rpc::RpcApi &api): api(api), rtjs(api)
    {}

protected:
    gcm::json::rpc::RpcApi &api;
    Rtjs rtjs;
};

extern "C" {

void *init(void *ptr_api) {
    gcm::json::rpc::RpcApi *api = (gcm::json::rpc::RpcApi *)ptr_api;
    return new RtjsServer(*api);
}

void stop(void *handler) {
    delete (RtjsServer *)handler;
}

} // enum C