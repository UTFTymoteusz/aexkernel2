#include "aex/net/linklayer.hpp"

namespace AEX::Net {
    LinkLayer::~LinkLayer() {}

    error_t LinkLayer::parse(const void*, size_t) {
        return error_t::ENOSYS;
    }
}