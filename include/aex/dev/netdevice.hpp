#pragma once

#include "aex/dev/device.hpp"
#include "aex/errno.hpp"
#include "aex/mem/smartptr.hpp"
#include "aex/net/ethernet.hpp"
#include "aex/net/ipv4.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Dev {
    class NetDevice : public Device {
        public:
        enum net_type_t : uint8_t {
            ETHERNET = 0x01,
        };

        Net::mac_addr ethernet_mac;

        Net::ipv4_addr ipv4_addr;
        Net::ipv4_addr ipv4_mask;
        Net::ipv4_addr ipv4_broadcast;

        net_type_t net_type;

        NetDevice(const char* name, net_type_t net_type);

        virtual ~NetDevice();

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
        void receive(const void* buffer, size_t len);

        /**
         * Sets the IPv4 address of the network interface and update the broadcast address.
         * @param addr IPv4 address.
         */
        void setIPv4Address(Net::ipv4_addr addr);

        /**
         * Sets the IPv4 mask of the network interface and update the broadcast address.
         * @param addr IPv4 mask.
         */
        void setIPv4Mask(Net::ipv4_addr addr);

        private:
    };

    Mem::SmartPointer<NetDevice> get_net_device(int id);
}