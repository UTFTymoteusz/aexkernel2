#include "aex/net/socket.hpp"

#include "aex/net/inetprotocol.hpp"
#include "aex/net/net.hpp"
#include "aex/printk.hpp"

namespace AEX::Net {
    Socket::~Socket() {}

    optional<Socket_SP> Socket::create(socket_domain_t domain, socket_type_t type,
                                       socket_protocol_t protocol) {
        switch (domain) {
        case socket_domain_t::AF_INET: {
            auto sock_try = inet_protocols[(uint8_t) protocol]->createSocket(type);
            if (!sock_try.has_value)
                return sock_try.error_code;

            return Socket_SP(sock_try.value);
        }
        case socket_domain_t::AF_UNIX:
            return error_t::EINVAL;
        default:
            return error_t::EINVAL;
        }
    }

    error_t Socket::connect(const sockaddr*) {
        return error_t::EINVAL;
    }

    error_t Socket::connect(ipv4_addr addr, uint16_t port) {
        sockaddr_inet aaa = {};

        aaa.domain = socket_domain_t::AF_INET;
        aaa.addr   = addr;
        aaa.port   = port;

        return connect((sockaddr*) &aaa);
    }

    error_t Socket::bind(const sockaddr*) {
        return error_t::EINVAL;
    }

    error_t Socket::bind(ipv4_addr addr, uint16_t port) {
        sockaddr_inet aaa = {};

        aaa.domain = socket_domain_t::AF_INET;
        aaa.addr   = addr;
        aaa.port   = port;

        return bind((sockaddr*) &aaa);
    }

    optional<size_t> Socket::sendTo(const void*, size_t, int, const sockaddr*) {
        return error_t::ENOSYS;
    }

    optional<size_t> Socket::receiveFrom(void*, size_t, int, sockaddr*) {
        return error_t::ENOSYS;
    }

    optional<size_t> Socket::send(const void* buffer, size_t len, int flags) {
        return sendTo(buffer, len, flags, nullptr);
    }

    optional<size_t> Socket::receive(void* buffer, size_t len, int flags) {
        return receiveFrom(buffer, len, flags, nullptr);
    }

    void Socket::close() {
        // return error_t::ENOSYS;
    }
}