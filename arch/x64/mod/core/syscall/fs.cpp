#include "aex/errno.hpp"
#include "aex/fs/file.hpp"
#include "aex/proc/process.hpp"
#include "aex/sys/syscall.hpp"
#include "aex/types.hpp"

#include "syscallids.h"
#include "types.hpp"
#include "usr.hpp"

using namespace AEX;
using namespace AEX::Proc;

optional<FS::File_SP> get_file(int fd);
optional<FS::File_SP> pop_file(int fd);

bool copy_and_canonize(char* buffer, const usr_char* usr_path);

int open(const usr_char* usr_path, int mode) {
    auto current = Proc::Process::current();
    char path_buffer[FS::MAX_PATH_LEN];

    if (!copy_and_canonize(path_buffer, usr_path)) {
        Thread::current()->errno = EINVAL;
        return -1;
    }

    auto file_try = FS::File::open(path_buffer, mode);
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

// this needs path verification
int chdir(const usr_char* path) {
    char path_buffer[FS::MAX_PATH_LEN];

    auto strlen_try = usr_strlen(path);
    if (!strlen_try) {
        Thread::current()->errno = EINVAL;
        return -1;
    }

    int len = min<int>(strlen_try.value, sizeof(path_buffer) - 1);

    auto memcpy_try = u2k_memcpy(path_buffer, path, len);
    if (!memcpy_try) {
        Thread::current()->errno = EINVAL;
        return -1;
    }

    path_buffer[len] = '\0';

    Proc::Process::current()->set_cwd(path_buffer);

    return 0;
}

char* getcwd(char* buffer, size_t buffer_len) {
    const char* cwd = Proc::Process::current()->get_cwd();
    int         len = strlen(cwd);

    if (len + 1 > buffer_len) {
        Thread::current()->errno = ERANGE;
        return nullptr;
    }

    k2u_memcpy(buffer, cwd, len + 1);

    return buffer;
}

int stat(const usr_char* usr_path, struct stat* usr_statbuf) {
    auto current = Proc::Process::current();
    char path_buffer[FS::MAX_PATH_LEN];

    if (!copy_and_canonize(path_buffer, usr_path)) {
        Thread::current()->errno = EINVAL;
        return -1;
    }

    struct stat ker_statbuf;

    auto info = FS::File::info(path_buffer);
    if (!info) {
        Thread::current()->errno = info.error_code;
        return -1;
    }

    ker_statbuf.st_dev = info.value.containing_dev_id;
    ker_statbuf.st_dev = 1;

    if (!u2k_memcpy(usr_statbuf, &ker_statbuf, sizeof(ker_statbuf))) {
        Thread::current()->errno = info.error_code;
        return -1;
    }

    return 0;
}

int access(const usr_char* usr_path, int mode) {
    char path_buffer[FS::MAX_PATH_LEN];

    if (!copy_and_canonize(path_buffer, usr_path)) {
        Thread::current()->errno = EINVAL;
        return -1;
    }

    auto info = FS::File::info(path_buffer);
    if (!info) {
        Thread::current()->errno = info.error_code;
        return -1;
    }

    // add permission checks

    return 0;
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
    table[SYS_ISATTY] = (void*) isatty;
    table[SYS_DUP]    = (void*) dup;
    table[SYS_DUP2]   = (void*) dup2;
    table[SYS_CHDIR]  = (void*) chdir;
    table[SYS_GETCWD] = (void*) getcwd;
    table[SYS_STAT]   = (void*) stat;
    table[SYS_ACCESS] = (void*) access;
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

bool copy_and_canonize(char buffer[FS::MAX_PATH_LEN], const usr_char* usr_path) {
    auto current    = Proc::Process::current();
    auto strlen_try = usr_strlen(usr_path);
    if (!strlen_try)
        return false;

    int len = min<int>(strlen_try.value + 1, FS::MAX_PATH_LEN);
    if (!u2k_memcpy(buffer, usr_path, len + 1))
        return false;

    buffer[len] = '\0';

    if (buffer[0] != '/') {
        char buffer_b[FS::MAX_PATH_LEN];
        FS::canonize_path(buffer, current->get_cwd(), buffer_b, FS::MAX_PATH_LEN);

        memcpy(buffer, buffer_b, FS::MAX_PATH_LEN);
    }

    return true;
}