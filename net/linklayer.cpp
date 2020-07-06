#include "aex/net/linklayer.hpp"

namespace AEX::Net {
    LinkLayer::~LinkLayer() {}

    error_t LinkLayer::parse(int, const void*, size_t) {
        return ENOSYS;
    }
}