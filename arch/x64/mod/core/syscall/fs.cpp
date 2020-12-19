#include "aex/errno.hpp"
#include "aex/fs/file.hpp"
#include "aex/proc/process.hpp"
#include "aex/sys/syscall.hpp"
#include "aex/types.hpp"

#include "syscallids.h"
#include "uptr.hpp"

using namespace AEX;
using namespace AEX::Proc;

optional<FS::File_SP> get_file(int fd);
optional<FS::File_SP> pop_file(int fd);

int open(const usr_char* usr_path, int mode) {
    auto current  = Proc::Process::current();
    auto file_try = FS::File::open(usr_path, mode);
    if (!file_try) {
        Thread::current()->errno = file_try.error_code;
        return -1;
    }

    current->files_lock.acquire();
    int fd = current->files.push(file_try.value);
    current->files_lock.release();

    return fd;
}

ssize_t read(int fd, usr_void* usr_buf, uint32_t count) {
    auto fd_try = get_file(fd);
    if (!fd_try) {
        Thread::current()->errno = fd_try.error_code;
        return -1;
    }

    auto read_try = fd_try.value->read((void*) usr_buf, count);
    if (!read_try) {
        Thread::current()->errno = read_try.error_code;
        return -1;
    }

    return read_try.value;
}

ssize_t write(int fd, const usr_void* usr_buf, uint32_t count) {
    auto fd_try = get_file(fd);
    if (!fd_try) {
        Thread::current()->errno = fd_try.error_code;
        return -1;
    }

    auto write_try = fd_try.value->write((void*) usr_buf, count);
    if (!write_try) {
        Thread::current()->errno = write_try.error_code;
        return -1;
    }

    return write_try.value;
}

int close(int fd) {
    auto fd_try = pop_file(fd);
    if (!fd_try) {
        Thread::current()->errno = fd_try.error_code;
        return -1;
    }

    return fd_try.value->close();
}

int dup(int fd) {
    auto current = Proc::Process::current();
    auto fd_try  = get_file(fd);
    if (!fd_try) {
        Thread::current()->errno = fd_try.error_code;
        return -1;
    }

    auto dup_try = fd_try.value->dup();
    if (!dup_try) {
        Thread::current()->errno = dup_try.error_code;
        return -1;
    }

    current->files_lock.acquire();
    int fd2 = current->files.push(dup_try.value);
    current->files_lock.release();

    return fd2;
}

int dup2(int srcfd, int dstfd) {
    auto current = Proc::Process::current();
    auto fd_try  = get_file(srcfd);
    if (!fd_try) {
        Thread::current()->errno = fd_try.error_code;
        return -1;
    }

    auto dup_try = fd_try.value->dup();
    if (!dup_try) {
        Thread::current()->errno = dup_try.error_code;
        return -1;
    }

    current->files_lock.acquire();
    current->files.set(dstfd, dup_try.value);
    current->files_lock.release();

    fd_try.value->close();

    return dstfd;
}

bool isatty(int fd) {
    auto fd_try = get_file(fd);
    if (!fd_try) {
        Thread::current()->errno = fd_try.error_code;
        return false;
    }

    bool tty                 = fd_try.value->isatty();
    Thread::current()->errno = tty ? ENONE : ENOTTY;

    return tty;
}

void register_fs() {
    auto table = Sys::default_table();

    table[SYS_OPEN]   = (void*) open;
    table[SYS_READ]   = (void*) read;
    table[SYS_WRITE]  = (void*) write;
    table[SYS_CLOSE]  = (void*) close;
    table[SYS_DUP]    = (void*) dup;
    table[SYS_DUP2]   = (void*) dup2;
    table[SYS_ISATTY] = (void*) isatty;
}

optional<FS::File_SP> get_file(int fd) {
    auto current = Proc::Process::current();
    auto scope   = current->files_lock.scope();

    if (!current->files.present(fd)) {
        PRINTK_DEBUG_WARN1("ebadf (%i)\n", fd);
        return EBADF;
    }

    return current->files[fd];
}

optional<FS::File_SP> pop_file(int fd) {
    auto current = Proc::Process::current();
    auto scope   = current->files_lock.scope();

    if (!current->files.present(fd)) {
        PRINTK_DEBUG_WARN1("ebadf (%i)\n", fd);
        return EBADF;
    }

    auto file = current->files[fd];
    current->files.erase(fd);

    return file;
}