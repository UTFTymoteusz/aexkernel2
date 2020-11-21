#include "aex/errno.hpp"
#include "aex/fs/file.hpp"
#include "aex/proc/process.hpp"
#include "aex/sys/syscall.hpp"

#include "syscallids.h"
#include "uptr.hpp"

using namespace AEX;

int open(const usr_char* usr_path, int mode) {
    auto current = Proc::Process::current();
    auto file    = FS::File::open(usr_path, mode);

    return current->files.push(file);
}

uint32_t read(int fd, usr_void* usr_buf, uint32_t count) {
    auto current = Proc::Process::current();

    if (!current->files.present(fd)) {
        PRINTK_DEBUG_WARN1("ebadf (%i)\n", fd);
        return EBADF;
    }

    auto file     = current->files[fd];
    auto read_try = file->read(usr_buf, count);

    return read_try.value;
}

uint32_t write(int fd, const usr_void* usr_buf, uint32_t count) {
    auto current = Proc::Process::current();

    if (!current->files.present(fd)) {
        PRINTK_DEBUG_WARN1("ebadf (%i)\n", fd);
        return EBADF;
    }

    auto file      = current->files[fd];
    auto write_try = file->write((void*) usr_buf, count);

    return write_try.value;
}


bool isatty(int fd) {
    auto current = Proc::Process::current();

    if (!current->files.present(fd)) {
        PRINTK_DEBUG_WARN1("ebadf (%i)\n", fd);
        return EBADF;
    }

    auto file = current->files[fd];
    return file->isatty();
}

void register_fs() {
    auto table = Sys::default_table();

    table[SYS_OPEN]   = (void*) open;
    table[SYS_READ]   = (void*) read;
    table[SYS_WRITE]  = (void*) write;
    table[SYS_ISATTY] = (void*) isatty;
}