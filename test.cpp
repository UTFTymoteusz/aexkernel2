#include "aex/dev.hpp"
#include "aex/net.hpp"
#include "aex/proc.hpp"

using namespace AEX;

struct blocc {
    Net::Socket_SP socket;
};

void test_server_handle(blocc* b) {
    auto sock2 = b->socket;
    delete b;

    char buffer[4];
    sock2->receive(buffer, 4, 0);

    sock2->send("HTTP/1.1 200 OK\r\nContent-Length: 16\r\n\r\n<h1>works</h1>\r\n", 56, 0);

    Proc::Thread::sleep(1000);
    sock2->shutdown(Net::SHUT_RDWR);

    Proc::Thread::sleep(1000);
    return;
}

extern "C" void context_test();

void test_client() {
    auto sock_try = Net::Socket::create(Net::AF_INET, Net::SOCK_STREAM, Net::IPROTO_TCP);
    auto sock     = sock_try.value;

    auto error = sock->connect(Net::ipv4_addr(127, 0, 0, 1), 7654);
    if (error) {
        printk("retrying client\n");
        sock->connect(Net::ipv4_addr(127, 0, 0, 1), 7654);
    }
    else
        printk("connected client\n");

    int total = 0;

    char buffer[64];

    auto ret = sock->send(
        "GET /login HTTP/1.1\r\nHost: 127.0.0.1\r\ncookie: "
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\r\n\r\n",
        745, 0);

    if (!ret)
        printk("socket send error: %s\n", strerror(ret));

    while (true) {
        auto ret = sock->receive(buffer, 63, 0);
        if (!ret) {
            printk("socket receive error: %s\n", strerror(ret));
            break;
        }

        total += ret.value;

        if (ret.value == 0) {
            printk("\nnothing more to read\n");
            break;
        }

        buffer[ret.value] = '\0';
        printk("%s", buffer, ret.value);

        if (total == 6165)
            break;
    }

    sock->shutdown(Net::SHUT_RDWR);

    Proc::Thread::sleep(5000);

    sock->close();
}

void test_server() {
    auto sock_try = Net::Socket::create(Net::AF_INET, Net::SOCK_STREAM, Net::IPROTO_TCP);
    if (!sock_try) {
        printk("failed to create() the socket: %s\n", strerror(sock_try));
        return;
    }

    auto sock = sock_try.value;

    auto error = sock->bind(Net::ipv4_addr(192, 168, 0, 23), 7654);
    if (error) {
        printk("failed to bind() the socket: %s\n", strerror(error));
        return;
    }

    error = sock->listen(2);
    if (error) {
        printk("failed to listen() the socket: %s\n", strerror(error));
        return;
    }

    for (size_t i = 0; i < 8; i++) {
        auto sock2_try = sock->accept();
        if (!sock2_try) {
            printk("failed to accept() the socket: %s\n", strerror(sock2_try));
            return;
        }

        auto sock2 = sock2_try.value;
        auto b     = new blocc();

        b->socket = sock2;

        printk("accepted\n");
        auto thread = Proc::threaded_call(test_server_handle, b);
        thread->detach();
    }

    Proc::Thread::sleep(1250);
}

void test_udp_client() {
    auto sock_try = Net::Socket::create(Net::AF_INET, Net::SOCK_DGRAM, Net::IPROTO_UDP);
    if (!sock_try) {
        printk("failed to create udp socket: %s\n", strerror(sock_try));
        return;
    }

    auto sock  = sock_try.value;
    auto error = sock->connect(Net::ipv4_addr(255, 255, 255, 255), 27015);
    if (error) {
        printk("failed to connect udp socket: %s\n", strerror(error));
        return;
    }

    for (size_t i = 0; i < 5; i++) {
        auto send_try = sock->send("\xFF\xFF\xFF\xFFTSource Engine Query", 25, 0);
        if (!send_try) {
            printk("failed to send via udp socket: %s\n", strerror(send_try));
            return;
        }

        char buffer[64] = {};

        auto receive_try = sock->receive(buffer, sizeof(buffer) - 1, 0);
        if (!receive_try) {
            printk("failed to receive via udp socket: %s\n", strerror(receive_try));
            return;
        }

        printk("udp received: %s\n", buffer);

        Proc::Thread::sleep(2500);
    }
}

void apple() {
    auto sock_try = Net::Socket::create(Net::AF_INET, Net::SOCK_STREAM, Net::IPROTO_TCP);
    if (!sock_try)
        kpanic("sock: %s", strerror(sock_try.error));

    auto sock  = sock_try.value;
    auto error = sock->connect(Net::ipv4_addr(192, 168, 0, 11), 8193);
    if (error)
        BROKEN;

    auto tty_wr = FS::File::open("/dev/tty0", FS::O_WRONLY);
    ASSERT(tty_wr);

    char buffer[4096];

    while (true) {
        auto aaa = sock->receive(buffer, 4096, 0);
        tty_wr.value->write(buffer, aaa.value);

        if (aaa.value == 0)
            break;
    }

    printk("Bad apple is done\n");
    Proc::Thread::sleep(2000);

    Dev::TTY::VTTYs[0]->clear();
    Dev::TTY::VTTYs[0]->setCursorX(0);
    Dev::TTY::VTTYs[0]->setCursorY(0);
}