#pragma once

#include "aex/fs/file.hpp"
#include "aex/net/ipv4.hpp"

namespace AEX::Net {
    enum socket_domain_t : uint16_t {
        AF_INET = 1,
        AF_UNIX = 3,
    };

    enum socket_type_t {
        SOCK_STREAM = 0,
        SOCK_DGRAM  = 1,
        SOCK_RAW    = 3,
    };

    enum socket_protocol_t {
        IPROTO_NONE = 0,
        IPROTO_TCP  = 1,
        IPROTO_UDP  = 2,
    };

    struct sockaddr {
        socket_domain_t domain;
        char            idk[14];
    };

    struct sockaddr_inet {
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

    class Socket;

    typedef Mem::SmartPointer<Socket> Socket_SP;

    class Socket : public FS::File {
        public:
        virtual ~Socket();

        optional<Mem::SmartPointer<FS::File>> open(const char* path)    = delete;
        optional<Mem::SmartPointer<FS::File>> opendir(const char* path) = delete;

        static optional<Socket_SP> create(socket_domain_t domain, socket_type_t type,
                                          socket_protocol_t protocol);

        virtual error_t connect(const sockaddr* addr);
        error_t         connect(ipv4_addr addr, uint16_t port);

        virtual error_t bind(const sockaddr* addr);
        error_t         bind(ipv4_addr addr, uint16_t port);

        virtual optional<size_t> sendTo(const void* buffer, size_t len, int flags,
                                        const sockaddr* dst_addr);
        virtual optional<size_t> receiveFrom(void* buffer, size_t len, int flags,
                                             sockaddr* src_addr);

        optional<size_t> send(const void* buffer, size_t len, int flags);
        optional<size_t> receive(void* buffer, size_t len, int flags);

        virtual void close();

        private:
    };
}