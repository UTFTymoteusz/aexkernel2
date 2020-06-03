#include "aex/net/net.hpp"

#include "aex/net/linklayer.hpp"

// The physical layer marks the buffer as busy\n

namespace AEX::Net {
    LinkLayer* link_layers[16];
    LinkLayer* null_link_layer;

    void init() {
        null_link_layer = new LinkLayer();

        for (size_t i = 0; i < sizeof(link_layers) / sizeof(LinkLayer*); i++)
            link_layers[i] = null_link_layer;
    }

    error_t register_link_layer(llayer_type_t type, LinkLayer* layer) {
        link_layers[type] = layer;

        return error_t::ENONE;
    }

    void parse(llayer_type_t type, const void* packet, size_t len) {
        link_layers[type]->parse(packet, len);
    }
}