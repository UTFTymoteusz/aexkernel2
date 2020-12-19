#include "aex/net.hpp"

#include "aex/assert.hpp"
#include "aex/fs.hpp"
#include "aex/math.hpp"
#include "aex/net/inetprotocol.hpp"
#include "aex/printk.hpp"

namespace AEX::Net {
    INetProtocol** inet_protocols;
    INetProtocol*  null_protocol;

    char* hostname;

    void read_hostname();

    void init() {
        printk(PRINTK_INIT "net: Initializing\n");

        inet_protocols = new INetProtocol*[256];

        null_protocol = new INetProtocol();
        for (size_t i = 0; i < 256; i++)
            inet_protocols[i] = null_protocol;

        read_hostname();

        printk("net: Hostname: %s\n", get_hostname());
        printk(PRINTK_OK "net: Initialized\n");
    }

    error_t register_inet_protocol(socket_protocol_t id, INetProtocol* protocol) {
        inet_protocols[(uint8_t) id] = protocol;

        return ENONE;
    }

    const char* get_hostname() {
        return hostname;
    }

    void set_hostname(const char* hostname_new) {
        int len = min<int>(strlen(hostname_new), 255);

        hostname = Mem::Heap::realloc(hostname, len + 1);
        strncpy(hostname, hostname_new, len + 1);
    }

    void read_hostname() {
        auto hostname_try = FS::File::open("/etc/hostname", FS::O_RD);
        if (!hostname_try.has_value)
            set_hostname("default");
        else {
            char buffer[256];

            auto read_try = hostname_try.value->read(buffer, sizeof(buffer) - 1);
            AEX_ASSERT(read_try.has_value);

            int len     = min<int>(read_try.value, sizeof(buffer) - 1);
            buffer[len] = '\0';

            set_hostname(buffer);
        }
    }
}