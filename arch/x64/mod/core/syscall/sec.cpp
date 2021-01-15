#include "aex/errno.hpp"
#include "aex/proc/process.hpp"
#include "aex/sys/syscall.hpp"
#include "aex/utility.hpp"

#include "syscallids.h"
#include "usr.hpp"

using namespace AEX;

Sec::gid_t getegid() {
    return Proc::Process::current()->eff_gid;
}

Sec::uid_t geteuid() {
    return Proc::Process::current()->eff_uid;
}

Sec::gid_t getgid() {
    return Proc::Process::current()->real_gid;
}

Sec::uid_t getuid() {
    return Proc::Process::current()->real_uid;
}

int setrgid(Sec::gid_t gid) {
    NOT_IMPLEMENTED;
}

int setruid(Sec::uid_t uid) {
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

int setegid(Sec::gid_t gid) {
    NOT_IMPLEMENTED;
}

int seteuid(Sec::uid_t uid) {
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

int setgid(Sec::gid_t gid) {
    NOT_IMPLEMENTED;
}

int setuid(Sec::uid_t uid) {
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

int setregid(Sec::gid_t rgid, Sec::gid_t egid) {
    NOT_IMPLEMENTED;
}

int setreuid(Sec::uid_t ruid, Sec::uid_t euid) {
    if (!inrange(ruid, -1, INT32_MAX) || !inrange(euid, -1, INT32_MAX)) {
        USR_ERRNO = EINVAL;
        return -1;
    }

    int ret = 0;

    if (ruid != -1)
        if (setruid(ruid) < 0)
            ret = -1;

    if (euid != -1)
        if (seteuid(euid) < 0)
            ret = -1;

    return ret;
}

void register_sec() {
    auto table = Sys::default_table();

    table[SYS_GETEGID] = (void*) getegid;
    table[SYS_GETEUID] = (void*) geteuid;
    table[SYS_GETGID]  = (void*) getgid;
    table[SYS_GETUID]  = (void*) getuid;

    table[SYS_SETEGID]  = (void*) setegid;
    table[SYS_SETEUID]  = (void*) seteuid;
    table[SYS_SETGID]   = (void*) setgid;
    table[SYS_SETUID]   = (void*) setuid;
    table[SYS_SETREGID] = (void*) setregid;
    table[SYS_SETREUID] = (void*) setreuid;
}