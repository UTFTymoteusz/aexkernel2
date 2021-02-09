#include "aex/net.hpp"
#include "aex/net/inetprotocol.hpp"
#include "aex/printk.hpp"

namespace AEX::Net {
    Socket::~Socket() {
        printk("socket is gone\n");
    }

    optional<Socket_SP> Socket::create(socket_domain_t domain, socket_type_t type,
                                       iproto_t protocol) {
        switch (domain) {
        case socket_domain_t::AF_INET: {
            auto sock_try = inet_protocols[(uint8_t) protocol]->createSocket(type);
            if (!sock_try)
                return sock_try.error_code;

            return sock_try.value;
        }
        case socket_domain_t::AF_UNIX:
            return EINVAL;
        default:
            return EINVAL;
        }
    }

    error_t Socket::connect(const sockaddr*) {
        return EINVAL;
    }

    error_t Socket::connect(ipv4_addr addr, uint16_t port) {
        sockaddr_inet aaa = {};

        aaa.domain = socket_domain_t::AF_INET;
        aaa.addr   = addr;
        aaa.port   = port;

        return connect((sockaddr*) &aaa);
    }

    error_t Socket::bind(const sockaddr*) {
        return EINVAL;
    }

    error_t Socket::bind(ipv4_addr addr, uint16_t port) {
        sockaddr_inet aaa = {};

        aaa.domain = socket_domain_t::AF_INET;
        aaa.addr   = addr;
        aaa.port   = port;

        return bind((sockaddr*) &aaa);
    }

    optional<Socket_SP> Socket::accept() {
        return ENOSYS;
    }

    error_t Socket::listen(int) {
        return ENOSYS;
    }

    optional<size_t> Socket::sendTo(const void*, size_t, int, const sockaddr*) {
        return ENOSYS;
    }

    optional<size_t> Socket::receiveFrom(void*, size_t, int, sockaddr*) {
        return ENOSYS;
    }

    optional<size_t> Socket::send(const void* buffer, size_t len, int flags) {
        return sendTo(buffer, len, flags, nullptr);
    }

    optional<size_t> Socket::receive(void* buffer, size_t len, int flags) {
        return receiveFrom(buffer, len, flags, nullptr);
    }

    error_t Socket::shutdown(int) {
        return ENOSYS;
    }

    error_t Socket::close() {
        return ENOSYS;
    }
}