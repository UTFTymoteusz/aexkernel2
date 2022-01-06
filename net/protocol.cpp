#include "aex/net/protocol.hpp"

#include "aex/errno.hpp"

namespace AEX::Net {
    optional<Socket_SP> Protocol::create() {
        return EPROTONOSUPPORT;
    }
};