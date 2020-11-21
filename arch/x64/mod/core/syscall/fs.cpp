#include "aex/errno.hpp"
#include "aex/fs/file.hpp"
#include "aex/proc/process.hpp"
#include "aex/sys/syscall.hpp"

#include "syscallids.h"
#include "uptr.hpp"

using namespace AEX;

optional<FS::File_SP> get_file(int fd);

int open(const usr_char* usr_path, int mode) {
    auto current = Proc::Process::current();
    auto file    = FS::File::open(usr_path, mode);

    current->files_lock.acquire();
    int fd = current->files.push(file);
    current->files_lock.release();

    return fd;
}

uint32_t read(int fd, usr_void* usr_buf, uint32_t count) {
    auto fd_try = get_file(fd);
    if (!fd_try)
        return fd_try.error_code;

    auto read_try = fd_try.value->read((void*) usr_buf, count);
    if (!read_try)
        return read_try.error_code;

    return read_try.value;
}

uint32_t write(int fd, const usr_void* usr_buf, uint32_t count) {
    auto fd_try = get_file(fd);
    if (!fd_try)
        return fd_try.error_code;

    auto write_try = fd_try.value->write((void*) usr_buf, count);
    if (!write_try)
        return write_try.error_code;

    return write_try.value;
}

bool isatty(int fd) {
    auto fd_try = get_file(fd);
    if (!fd_try)
        return fd_try.error_code;

    return fd_try.value->isatty();
}

void register_fs() {
    auto table = Sys::default_table();

    table[SYS_OPEN]   = (void*) open;
    table[SYS_READ]   = (void*) read;
    table[SYS_WRITE]  = (void*) write;
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