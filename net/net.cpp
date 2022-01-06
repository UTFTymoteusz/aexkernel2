#include "aex/net.hpp"

#include "aex/assert.hpp"
#include "aex/fs.hpp"
#include "aex/math.hpp"
#include "aex/net/domain.hpp"
#include "aex/printk.hpp"
#include "aex/spinlock.hpp"

namespace AEX::Net {
    Domain** domains;

    char*    hostname;
    Spinlock hostname_lock;

    void read_hostname();

    void init() {
        printk(INIT "net: Initializing\n");

        domains = new Domain*[256];

        read_hostname();

        char buffer[256];

        printk("net: Hostname: %s\n", get_hostname(buffer, sizeof(buffer)).value);
        printk(OK "net: Initialized\n");
    }

    error_t register_domain(domain_t af, Domain* domain) {
        domains[(uint8_t) af] = domain;
        return ENONE;
    }

    optional<char*> get_hostname(char* buffer, size_t len) {
        SCOPE(hostname_lock);

        if (strlen(hostname) + 1 > len)
            return EINVAL;

        strlcpy(buffer, hostname, len + 1);
        return buffer;
    }

    void set_hostname(const char* hostname_new) {
        SCOPE(hostname_lock);
        size_t len = min<size_t>(strlen(hostname_new), 255);

        hostname = Mem::Heap::realloc(hostname, len + 1);
        strlcpy(hostname, hostname_new, len + 1);
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