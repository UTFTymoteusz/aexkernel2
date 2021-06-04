
#include "aex/net.hpp"

#include "syscallids.h"
#include "usr.hpp"

using namespace AEX;

int gethostname(char* name, size_t len) {
    USR_ENSURE(len <= 256);

    char buffer[len];

    USR_ENSURE_OPT(Net::get_hostname(buffer, sizeof(len)));
    USR_ENSURE_OPT(k2u_memcpy(name, buffer, len));

    return 0;
}

__attribute__((optimize("O2"))) void register_net() {
    auto table = Sys::default_table();

    table[SYS_GETHOSTNAME] = (void*) gethostname;
}