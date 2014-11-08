#include <gcm/socket/socket.h>

namespace s = gcm::socket;

extern "C" {

void handle(s::ClientSocket<s::AnyIpAddress> client) {
    client << "Hello" << std::endl;
    sleep(5);
    client << "world!" << std::endl;
}

} // enum C