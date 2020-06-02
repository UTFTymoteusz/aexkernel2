#include "aex/dev/net.hpp"

namespace AEX::Dev {
    Net::Net(const char* name, net_type_t net_type) : Device(name, type_t::NET) {
        this->net_type = net_type;
    }

    Net::~Net() {}

    error_t Net::send(const void*, size_t) {
        return error_t::ENOSYS;
    }
}