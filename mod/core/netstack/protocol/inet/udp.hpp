#pragma once

#include "aex/errno.hpp"
#include "aex/ipc/event.hpp"
#include "aex/mem.hpp"
#include "aex/net.hpp"
#include "aex/net/protocol.hpp"
#include "aex/spinlock.hpp"

#include <stdint.h>

namespace NetStack {
    static constexpr auto UDP_SOCKET_BUFFER_SIZE = 1048576;

    class UDPSocket;

    class UDPProtocol : public AEX::Net::Protocol {
        public:
        static AEX::Spinlock                        sockets_lock;
        static AEX::Mem::Vector<UDPSocket*, 32, 32> sockets;

        static void init();

        AEX::optional<AEX::Net::Socket_SP> create();

        static void packetReceived(AEX::Net::ipv4_addr src, uint16_t src_port,
                                   AEX::Net::ipv4_addr dst, uint16_t dst_port,
                                   const uint8_t* buffer, uint16_t len);

        static uint16_t allocateDynamicPort();
        static bool     allocatePort(uint16_t port);
        static void     freePort(uint16_t port);

        private:
        static AEX::Spinlock m_ports_lock;
        static uint32_t*     m_port_bitmap;
        static uint16_t      m_port_dynamic_last;
    };
}