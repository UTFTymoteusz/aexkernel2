#pragma once

#include "aex/errno.hpp"
#include "aex/net/net.hpp"

namespace AEX::Net {
    class LinkLayer {
      public:
        virtual ~LinkLayer();

        // virtual void encapsulate(void* buffer, size_t len);

        /**
         * Called by the network stack to parse a packet.
         * @param packet_ptr Packet parse pointer.
         * @param len Remaining parse length.
         * @returns The result.
         */
        virtual error_t parse(const void* packet_ptr, size_t len);

      private:
    };
}