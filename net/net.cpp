#include "aex/net.hpp"

#include "aex/net/inetprotocol.hpp"
#include "aex/printk.hpp"

namespace AEX::Net {
    INetProtocol** inet_protocols;
    INetProtocol*  null_protocol;

    void init() {
        printk(PRINTK_INIT "net: Initializing\n");

        inet_protocols = new INetProtocol*[256];

        null_protocol = new INetProtocol();
        for (size_t i = 0; i < 256; i++)
            inet_protocols[i] = null_protocol;

        printk(PRINTK_OK "net: Initialized\n");
    }

    error_t register_inet_protocol(socket_protocol_t id, INetProtocol* protocol) {
        inet_protocols[(uint8_t) id] = protocol;

        return ENONE;
    }
}