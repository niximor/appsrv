#include <gcm/appsrv/interface.h>
#include <gcm/socket/socket.h>
#include <gcm/logging/logging.h>

#include <gcm/socket/http.h>

namespace s = gcm::socket;
namespace l = gcm::logging;

using namespace gcm::socket::http;

class HttpHandler: public gcm::appsrv::Handler {
public:
    void handle(s::ConnectedSocket<s::AnyIpAddress> &&client) {
        auto &log = l::getLogger("client");
        auto &addr = client.get_client_address();

        DEBUG(log) << "Handle client " << addr.get_ip() << ":" << addr.get_port() << ".";

        try {
            client << s::ascii;

            HttpRequest req;
            req.parse(client);

            auto response = req.get_response(client);

            response << "Hello world!";

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

void *init() {
    return new HttpHandler();
}

void stop(void *handler) {
    delete (HttpHandler *)handler;
}

} // enum C