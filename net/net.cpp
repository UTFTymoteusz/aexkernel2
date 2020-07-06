#include "aex/net.hpp"

#include "aex/net/inetprotocol.hpp"
#include "aex/net/linklayer.hpp"
#include "aex/printk.hpp"

namespace AEX::Net {
    LinkLayer* link_layers[16];
    LinkLayer* null_link_layer;

    INetProtocol** inet_protocols;
    INetProtocol*  null_protocol;

    void init() {
        printk(PRINTK_INIT "net: Initializing\n");

        inet_protocols = new INetProtocol*[256];

        null_link_layer = new LinkLayer();
        null_protocol   = new INetProtocol();

        for (size_t i = 0; i < sizeof(link_layers) / sizeof(LinkLayer*); i++)
            link_layers[i] = null_link_layer;

        for (size_t i = 0; i < 256; i++)
            inet_protocols[i] = null_protocol;

        printk(PRINTK_OK "net: Link layer interface initialized\n");
        printk(PRINTK_OK "net: Initialized\n");
    }

    error_t register_link_layer(link_type_t type, LinkLayer* layer) {
        link_layers[type] = layer;

        return ENONE;
    }

    void parse(int device_id, link_type_t type, const void* packet, size_t len) {
        link_layers[type]->parse(device_id, packet, len);
    }

    error_t register_inet_protocol(socket_protocol_t id, INetProtocol* protocol) {
        inet_protocols[(uint8_t) id] = protocol;

        return ENONE;
    }
}