#include <gcm/appsrv/interface.h>
#include <gcm/socket/socket.h>
#include <gcm/logging/logging.h>

#include <gcm/socket/http.h>

namespace s = gcm::socket;
namespace l = gcm::logging;

using namespace gcm::socket::http;

class HttpHandler: public gcm::appsrv::Handler {
protected:
    gcm::appsrv::ServerApi &api;

public:
    HttpHandler(gcm::appsrv::ServerApi &api): api(api)
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
            response.set_header("Content-Type", "text/html");

            response << "Hello world!<br />";
            response << "Interface " << api.handler_name << " statistics: <br />";
            response << "Total requests: " << api.handler_stats.req_received << "<br />";
            response << "Handled: " << api.handler_stats.req_handled << "<br />";
            response << "Error: " << api.handler_stats.req_error << "<br />";

            // Write empty body if there is nothing already written.
            response << "";
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
    return new HttpHandler(*api);
}

void stop(void *handler) {
    delete (HttpHandler *)handler;
}

} // enum C