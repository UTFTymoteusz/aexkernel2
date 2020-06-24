#pragma once

#include "aex/errno.hpp"
#include "aex/net/net.hpp"

namespace AEX::Net {
    class LinkLayer {
        public:
        virtual ~LinkLayer();

        // virtual void encapsulate(void* buffer, size_t len);

        /**
         * Called by Net::parse() to parse a packet.
         * @param device_id The recipient device ID.
         * @param packet_ptr Packet parse pointer.
         * @param len Remaining parse length.
         * @returns The result.
         */
        virtual error_t parse(int device_id, const void* packet_ptr, size_t len);

        private:
    };
}