
#include "aex/net.hpp"

#include "aex/mem/usr.hpp"

#include "syscallids.h"
#include "usr.hpp"

using namespace AEX;
using namespace AEX::Mem;

int sys_gethostname(char* name, size_t len) {
    USR_ENSURE(len <= 256);

    char buffer[len];

    USR_ENSURE_OPT(Net::get_hostname(buffer, sizeof(len)));
    USR_ENSURE_OPT(k2u_memcpy(name, buffer, len));

    return 0;
}

O0 void register_net(Sys::syscall_t* table) {
    table[SYS_GETHOSTNAME] = (void*) sys_gethostname;
}