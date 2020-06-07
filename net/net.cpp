#include "aex/net/net.hpp"

#include "aex/net/linklayer.hpp"
#include "aex/printk.hpp"

namespace AEX::Net {
    LinkLayer* link_layers[16];
    LinkLayer* null_link_layer;

    void init() {
        null_link_layer = new LinkLayer();

        for (size_t i = 0; i < sizeof(link_layers) / sizeof(LinkLayer*); i++)
            link_layers[i] = null_link_layer;

        printk(PRINTK_OK "net: Link layer initialized\n");
    }

    error_t register_link_layer(link_type_t type, LinkLayer* layer) {
        link_layers[type] = layer;

        return error_t::ENONE;
    }

    void parse(int device_id, link_type_t type, const void* packet, size_t len) {
        link_layers[type]->parse(device_id, packet, len);
    }
}