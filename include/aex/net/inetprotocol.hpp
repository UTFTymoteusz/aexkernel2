#pragma once

#include "aex/net/socket.hpp"

namespace AEX::Net {
    class INetProtocol {
        public:
        virtual optional<Socket_SP> createSocket(socket_type_t type);
    };
}