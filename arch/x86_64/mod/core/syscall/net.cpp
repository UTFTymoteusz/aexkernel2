
#include "aex/net.hpp"

#include "syscallids.h"
#include "usr.hpp"

using namespace AEX;

int gethostname(char* name, size_t len) {
    if (len > 256) {
        USR_ERRNO = EINVAL;
        return -1;
    }

    char buffer[len];
    auto hostname = Net::get_hostname(buffer, sizeof(len));
    if (!hostname) {
        USR_ERRNO = hostname.error_code;
        return -1;
    }

    if (!k2u_memcpy(name, buffer, len)) {
        USR_ERRNO = EINVAL;
        return -1;
    }

    return 0;
}

void register_net() {
    auto table = Sys::default_table();

    table[SYS_GETHOSTNAME] = (void*) gethostname;
}