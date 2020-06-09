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
    struct ipv4_info {
        Net::mac_addr  mac;
        Net::ipv4_addr addr;
        Net::ipv4_addr mask;
        Net::ipv4_addr broadcast;
        Net::ipv4_addr gateway;
    };

    struct netdev_info {
        ipv4_info ipv4;
        char      boi[234];
    };

    static_assert(sizeof(netdev_info) == 256);

    class NetDevice : public Device {
        public:
        Net::link_type_t link_type;

        netdev_info info;

        int metric;

        NetDevice(const char* name, Net::link_type_t link_type);

        virtual ~NetDevice();

        /**
         * Called by the network stack to send a packet.
         * @param buffer Source buffer, the device should copy this over.
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

        void setMetric(int metric);

        private:
    };

    typedef Mem::SmartPointer<NetDevice> NetDevice_SP;

    NetDevice_SP get_net_device(int id);
}