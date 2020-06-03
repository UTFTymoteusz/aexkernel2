#include "aex/dev/net.hpp"

#include "aex/net/linklayer.hpp"

namespace AEX::Dev {
    Net::Net(const char* name, net_type_t net_type) : Device(name, type_t::NET) {
        this->net_type = net_type;
    }

    Net::~Net() {}

    error_t Net::send(const void*, size_t) {
        return error_t::ENOSYS;
    }

    void Net::receive(const void* buffer, size_t len) {
        switch (net_type) {
        case net_type_t::ETHERNET:
            AEX::Net::parse(AEX::Net::llayer_type_t::ETHERNET, buffer, len);
            break;
        }
    }
}