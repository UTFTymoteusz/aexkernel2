#pragma once

#include "aex/dev/device.hpp"
#include "aex/errno.hpp"
#include "aex/mem/smartptr.hpp"
#include "aex/net/ethernet.hpp"
#include "aex/net/ipv4.hpp"
#include "aex/net/net.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Dev {
    class NetDevice : public Device {
        public:
        Net::mac_addr ethernet_mac;

        Net::ipv4_addr ipv4_addr;
        Net::ipv4_addr ipv4_mask;
        Net::ipv4_addr ipv4_broadcast;
        Net::ipv4_addr ipv4_gateway;

        Net::link_type_t link_type;

        NetDevice(const char* name, Net::link_type_t link_type);

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
         * Sets the IPv4 address of the network interface and updates the broadcast address.
         * @param addr IPv4 address.
         */
        void setIPv4Address(Net::ipv4_addr addr);

        /**
         * Sets the IPv4 mask of the network interface and updates the broadcast address.
         * @param addr IPv4 mask.
         */
        void setIPv4Mask(Net::ipv4_addr addr);

        /**
         * Sets the IPv4 gateway of the network interface.
         * @param addr IPv4 gateway.
         */
        void setIPv4Gateway(Net::ipv4_addr addr);

        private:
    };

    typedef Mem::SmartPointer<NetDevice> NetDevice_SP;

    NetDevice_SP get_net_device(int id);
}