#pragma once

#include "aex/net/socket.hpp"
#include "aex/utility.hpp"

namespace AEX::Net {
    class API INetProtocol {
        public:
        virtual optional<Socket_SP> createSocket(socket_type_t type);
    };
}