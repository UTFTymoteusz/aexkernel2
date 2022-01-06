#include "aex/net.hpp"
#include "aex/net/domain.hpp"
#include "aex/net/protocol.hpp"
#include "aex/printk.hpp"

namespace AEX::Net {
    Socket::~Socket() {
        printk("socket is gone\n");
    }

    optional<Socket_SP> Socket::create(domain_t domain, socket_type_t type, int proto) {
        if (!domains[(uint8_t) domain])
            return EAFNOSUPPORT;

        return domains[(uint8_t) domain]->create(type, proto);
    }

    error_t Socket::connect(const sockaddr*) {
        return EINVAL;
    }

    error_t Socket::connect(ipv4_addr addr, uint16_t port) {
        sockaddr_inet aaa = {};

        aaa.domain = domain_t::AF_INET;
        aaa.addr   = addr;
        aaa.port   = port;

        return connect((sockaddr*) &aaa);
    }

    error_t Socket::bind(const sockaddr*) {
        return EINVAL;
    }

    error_t Socket::bind(ipv4_addr addr, uint16_t port) {
        sockaddr_inet aaa = {};

        aaa.domain = domain_t::AF_INET;
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

    optional<ssize_t> Socket::sendto(const void*, size_t, int, const sockaddr*) {
        return ENOSYS;
    }

    optional<ssize_t> Socket::receivefrom(void*, size_t, int, sockaddr*) {
        return ENOSYS;
    }

    optional<ssize_t> Socket::send(const void* buffer, size_t len, int flags) {
        return sendto(buffer, len, flags, nullptr);
    }

    optional<ssize_t> Socket::receive(void* buffer, size_t len, int flags) {
        return receivefrom(buffer, len, flags, nullptr);
    }

    error_t Socket::shutdown(int) {
        return ENOSYS;
    }

    error_t Socket::close() {
        return ENOSYS;
    }
}