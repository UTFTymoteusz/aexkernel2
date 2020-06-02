#pragma once

#include "aex/dev/device.hpp"
#include "aex/errno.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Dev {
    class Net : public Device {
      public:
        enum net_type_t : uint8_t {
            ETHERNET = 0x01,
        };

        int     address_len;
        uint8_t address[16];

        net_type_t net_type;

        Net(const char* name, net_type_t net_type);

        virtual ~Net();

        /**
         * Called by the network stack to send a packet.
         * @param buffer Source buffer, this should be copied over.
         * @param buffer Buffer length.
         * @returns Error code.
         */
        virtual error_t send(const void* buffer, size_t len);

        /**
         * Notifies the network stack of a packet reception. The device should call this method upon
         * a reception of a packet.
         * @param buffer Buffer.
         * @param buffer Buffer length.
         */
        static void receive(const void* buffer, size_t len);

      private:
    };
}