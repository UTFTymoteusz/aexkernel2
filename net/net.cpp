#include "aex/net.hpp"

#include "aex/assert.hpp"
#include "aex/fs.hpp"
#include "aex/math.hpp"
#include "aex/net/inetprotocol.hpp"
#include "aex/printk.hpp"
#include "aex/spinlock.hpp"

namespace AEX::Net {
    INetProtocol** inet_protocols;
    INetProtocol*  null_protocol;

    char*    hostname;
    Spinlock hostname_lock;

    void read_hostname();

    void init() {
        printk(INIT "net: Initializing\n");

        inet_protocols = new INetProtocol*[256];
        null_protocol  = new INetProtocol();

        for (size_t i = 0; i < 256; i++)
            inet_protocols[i] = null_protocol;

        read_hostname();

        char buffer[256];

        printk("net: Hostname: %s\n", get_hostname(buffer, sizeof(buffer)).value);
        printk(OK "net: Initialized\n");
    }

    error_t register_inet_protocol(iproto_t id, INetProtocol* protocol) {
        inet_protocols[(uint8_t) id] = protocol;

        return ENONE;
    }

    optional<char*> get_hostname(char* buffer, size_t len) {
        SCOPE(hostname_lock);

        if (strlen(hostname) + 1 > len)
            return EINVAL;

        return strncpy(buffer, hostname, len + 1);
    }

    void set_hostname(const char* hostname_new) {
        SCOPE(hostname_lock);
        size_t len = min<size_t>(strlen(hostname_new), 255);

        hostname = Mem::Heap::realloc(hostname, len + 1);
        strncpy(hostname, hostname_new, len + 1);
    }

    void read_hostname() {
        auto hostname_try = FS::File::open("/etc/hostname", FS::O_RDONLY);
        if (!hostname_try) {
            set_hostname("default");
        }
        else {
            char buffer[256];

            auto read_try = hostname_try.value->read(buffer, sizeof(buffer) - 1);
            AEX_ASSERT(read_try);

            int len     = min<int>(read_try.value, sizeof(buffer) - 1);
            buffer[len] = '\0';

            set_hostname(buffer);
        }
    }
}