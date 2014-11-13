#include <gcm/appsrv/interface.h>
#include <gcm/socket/socket.h>
#include <gcm/socket/http.h>
#include <gcm/logging/logging.h>
#include <gcm/thread/pool.h>
#include <gcm/json/json.h>
#include <gcm/json/rpc.h>

#include <vector>
#include <memory>
#include <stdexcept>

namespace s = gcm::socket;
namespace l = gcm::logging;

using namespace gcm::socket::http;

class JsonHttpHandler: public gcm::appsrv::Handler {
protected:
    gcm::appsrv::ServerApi &api;
    gcm::json::rpc::Rpc json;

public:
    JsonHttpHandler(gcm::appsrv::ServerApi &api): api(api)
    {}

    void handle(s::ConnectedSocket<s::AnyIpAddress> &&client) {
        auto &log = l::getLogger(api.handler_name);
        auto &addr = client.get_client_address();

        DEBUG(log) << "Handle client " << addr.get_ip() << ":" << addr.get_port() << ".";

        try {
            client << s::ascii;

            HttpRequest req;
            req.parse(client);

            auto response = req.get_response(client);
            response.set_header("Content-Type", "application/json");

            // TODO: Parse JSON from request.
            auto p = json.add_work(1, std::string("system.listMethods"), gcm::json::Array());

            auto resp = p->get();
            std::string out{resp->to_string()};

            response.set_header("Content-Length", std::to_string(out.size()));
            response << out;

            /*response << "Hello world!<br />";
            response << "Interface " << api.handler_name << " statistics: <br />";
            response << "Total requests: " << api.handler_stats.req_received << "<br />";
            response << "Handled: " << api.handler_stats.req_handled << "<br />";
            response << "Error: " << api.handler_stats.req_error << "<br />";*/
        } catch (HttpException &e) {
            e.write(client);
            ERROR(log) << addr.get_ip() << ":" << addr.get_port() << " " << e.get_status() << " " << e.what();
        }

        DEBUG(log) << "Client " << addr.get_ip() << ":" << addr.get_port() << " handled.";
    }
};

extern "C" {

void *init(void *ptr_api) {
    gcm::appsrv::ServerApi *api = (gcm::appsrv::ServerApi *)ptr_api;
    return new JsonHttpHandler(*api);
}

void stop(void *handler) {
    delete (HttpHandler *)handler;
}

} // enum C