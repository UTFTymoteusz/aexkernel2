#include "aex/errno.hpp"
#include "aex/mem/usr.hpp"
#include "aex/proc/process.hpp"
#include "aex/sys/syscall.hpp"
#include "aex/utility.hpp"

#include "syscallids.h"
#include "usr.hpp"

using namespace AEX;
using namespace AEX::Mem;

Sec::gid_t sys_getegid() {
    return Proc::Process::current()->eff_gid;
}

Sec::uid_t sys_geteuid() {
    return Proc::Process::current()->eff_uid;
}

Sec::gid_t sys_getgid() {
    return Proc::Process::current()->real_gid;
}

Sec::uid_t sys_getuid() {
    return Proc::Process::current()->real_uid;
}

int sys_setrgid(Sec::gid_t gid) NOT_IMPLEMENTED;

int sys_setruid(Sec::uid_t uid) {
    auto current = Proc::Process::current();

    if (current->eff_uid == 0) {
        current->real_uid = uid;
        return 0;
    }

    if (uid == current->real_uid || uid == current->saved_uid) {
        current->real_uid = uid;
        return 0;
    }

    USR_ERRNO = EPERM;
    return -1;
}

int sys_setegid(Sec::gid_t gid) NOT_IMPLEMENTED;

int sys_seteuid(Sec::uid_t uid) {
    auto current = Proc::Process::current();

    // or if the process has appropriate privileges,
    if (current->eff_uid == 0) {
        current->eff_uid = uid;
        return 0;
    }

    // If uid is equal to the real user ID or the saved set-user-ID,
    if (uid == current->real_uid || uid == current->saved_uid) {
        current->eff_uid = uid;
        return 0;
    }

    USR_ERRNO = EPERM;
    return -1;
}

int sys_setgid(Sec::gid_t gid) NOT_IMPLEMENTED;

int sys_setuid(Sec::uid_t uid) {
    auto current = Proc::Process::current();

    // If the process has appropriate privileges,
    if (current->eff_uid == 0) {
        current->real_uid  = uid;
        current->eff_uid   = uid;
        current->saved_uid = uid;

        return 0;
    }

    // If the process does not have appropriate privileges,
    // but uid is equal to the real user ID or the saved set-user-ID,
    if (uid == current->real_uid || uid == current->saved_uid) {
        current->eff_uid = uid;
        return 0;
    }

    USR_ERRNO = EPERM;
    return -1;
}

int sys_setregid(Sec::gid_t rgid, Sec::gid_t egid) NOT_IMPLEMENTED;

int sys_setreuid(Sec::uid_t ruid, Sec::uid_t euid) {
    if (!inrange(ruid, -1, INT32_MAX) || !inrange(euid, -1, INT32_MAX)) {
        USR_ERRNO = EINVAL;
        return -1;
    }

    int ret = 0;

    if (ruid != -1)
        if (sys_setruid(ruid) < 0)
            ret = -1;

    if (euid != -1)
        if (sys_seteuid(euid) < 0)
            ret = -1;

    return ret;
}

O0 void register_sec(Sys::syscall_t* table) {
    table[SYS_GETEGID] = (void*) sys_getegid;
    table[SYS_GETEUID] = (void*) sys_geteuid;
    table[SYS_GETGID]  = (void*) sys_getgid;
    table[SYS_GETUID]  = (void*) sys_getuid;

    table[SYS_SETEGID]  = (void*) sys_setegid;
    table[SYS_SETEUID]  = (void*) sys_seteuid;
    table[SYS_SETGID]   = (void*) sys_setgid;
    table[SYS_SETUID]   = (void*) sys_setuid;
    table[SYS_SETREGID] = (void*) sys_setregid;
    table[SYS_SETREUID] = (void*) sys_setreuid;
}