#pragma once

#include "aex/net/socket.hpp"
#include "aex/utility.hpp"

namespace AEX::Net {
    class API Protocol {
        public:
        virtual optional<Socket_SP> create();
    };
}