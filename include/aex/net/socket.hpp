#pragma once

#include "aex/fs/file.hpp"
#include "aex/net/ipv4.hpp"
#include "aex/net/types.hpp"
#include "aex/utility.hpp"

namespace AEX::Net {
    enum socket_domain_t : uint16_t {
        AF_UNSPEC = 0,
        AF_INET   = 1,
        AF_INET6  = 2,
        AF_UNIX   = 3,
    };

    enum socket_type_t {
        SOCK_STREAM    = 0,
        SOCK_DGRAM     = 1,
        SOCK_SEQPACKET = 2,
        SOCK_RAW       = 3,
    };

    enum iproto_t {
        IPROTO_NONE = 0,
        IPROTO_IP   = 1,
        IPROTO_IPV6 = 2,
        IPROTO_ICMP = 3,
        IPROTO_RAW  = 4,
        IPROTO_TCP  = 5,
        IPROTO_UDP  = 6,
    };

    enum socket_flag_t {
        MSG_WAITALL = 0x01,
    };

    enum socket_shutdown_flag_t {
        SHUT_RD   = 0x01,
        SHUT_WR   = 0x02,
        SHUT_RDWR = 0x03,
    };

    struct API sockaddr {
        socket_domain_t domain;
        char            idk[14];
    };

    struct API sockaddr_inet {
        socket_domain_t domain;
        uint16_t        port;
        ipv4_addr       addr;
        char            idk[8];

        sockaddr_inet() {}

        sockaddr_inet(ipv4_addr addr, uint16_t port) {
            this->addr = addr;
            this->port = port;
        }
    };

    static_assert(sizeof(sockaddr) == sizeof(sockaddr_inet));
    static_assert(sizeof(sockaddr) == 16);

    class API Socket : public FS::File {
        public:
        virtual ~Socket();

        optional<Mem::SmartPointer<FS::File>> open(const char* path) = delete;

        static optional<Socket_SP> create(socket_domain_t domain, socket_type_t type,
                                          iproto_t protocol);

        virtual error_t connect(const sockaddr* addr);
        error_t         connect(ipv4_addr addr, uint16_t port);

        virtual error_t bind(const sockaddr* addr);
        error_t         bind(ipv4_addr addr, uint16_t port);

        virtual error_t             listen(int backlog);
        virtual optional<Socket_SP> accept();

        virtual optional<size_t> sendTo(const void* buffer, size_t len, int flags,
                                        const sockaddr* dst_addr);
        virtual optional<size_t> receiveFrom(void* buffer, size_t len, int flags,
                                             sockaddr* src_addr);

        optional<size_t> send(const void* buffer, size_t len, int flags);
        optional<size_t> receive(void* buffer, size_t len, int flags);

        virtual error_t shutdown(int how);
        virtual error_t close();
    };
}