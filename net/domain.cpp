#include "aex/net/domain.hpp"

#include "aex/assert.hpp"
#include "aex/errno.hpp"
#include "aex/net.hpp"
#include "aex/net/protocol.hpp"
#include "aex/utility.hpp"

namespace AEX::Net {
    optional<Socket_SP> Domain::create(socket_type_t type, int) {
        for (auto& dproto_val : m_dproto) {
            if (dproto_val.type == type)
                return dproto_val.proto->create();
        }

        return EINVAL;
    }

    void Domain::push(socket_type_t type, int protoid, Protocol* proto) {
        ASSERT(m_type_counts[type]++ == 0);
        m_dproto.push({.type = type, .protoid = protoid, .proto = proto});
    }
}