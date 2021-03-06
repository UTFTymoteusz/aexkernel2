#include "aex/net/inetprotocol.hpp"

#include "aex/errno.hpp"
#include "aex/net.hpp"
#include "aex/printk.hpp"

namespace AEX::Net {
    optional<Socket_SP> INetProtocol::createSocket(socket_type_t) {
        return EPROTONOSUPPORT;
    }
}