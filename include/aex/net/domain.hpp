#pragma once

#include "aex/mem/vector.hpp"
#include "aex/net/protocol.hpp"
#include "aex/net/socket.hpp"
#include "aex/utility.hpp"

namespace AEX::Net {
    class API Domain {
        public:
        optional<Socket_SP> create(socket_type_t type, int protoid);
        void                push(socket_type_t type, int protoid, Protocol* proto);

        private:
        struct dproto {
            socket_type_t type;
            int           protoid;
            Protocol*     proto;
        };

        uint8_t             m_type_counts[8];
        Mem::Vector<dproto> m_dproto;
    };
}