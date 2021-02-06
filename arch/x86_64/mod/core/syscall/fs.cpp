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
        USR_ERRNO = EINVAL;
        return -1;
    }

    auto file_try = FS::File::open(path_buffer, mode);
    if (!file_try) {
        USR_ERRNO = file_try.error_code;
        return -1;
    }

    current->files_lock.acquire();
    int fd = current->files.push(file_try.value);
    current->files_lock.release();

    return fd;
}

ssize_t read(int fd, usr_void* usr_buf, size_t count) {
    auto fd_try = get_file(fd);
    if (!fd_try) {
        USR_ERRNO = fd_try.error_code;
        return -1;
    }

    auto usr_buf_c = (usr_uint8_t*) usr_buf;

    uint8_t buffer[1024];
    size_t  read = 0;

    for (size_t i = 0; i < count; i += sizeof(buffer)) {
        int len = min<size_t>(count - i, sizeof(buffer));

        auto read_try = fd_try.value->read(buffer, len);
        if (!read_try) {
            USR_ERRNO = read_try.error_code;
            return -1;
        }

        if (!k2u_memcpy(&usr_buf_c[i], buffer, len)) {
            USR_ERRNO = EINVAL;
            return -1;
        }

        read += len;
    }

    return read;
}

ssize_t write(int fd, const usr_void* usr_buf, size_t count) {
    auto fd_try = get_file(fd);
    if (!fd_try) {
        USR_ERRNO = fd_try.error_code;
        return -1;
    }

    auto usr_buf_c = (const usr_uint8_t*) usr_buf;

    uint8_t buffer[1024];
    size_t  written = 0;

    for (size_t i = 0; i < count; i += sizeof(buffer)) {
        int len = min<size_t>(count - i, sizeof(buffer));

        if (!u2k_memcpy(buffer, &usr_buf_c[i], len)) {
            USR_ERRNO = EINVAL;
            return -1;
        }

        auto write_try = fd_try.value->write(buffer, len);
        if (!write_try) {
            USR_ERRNO = write_try.error_code;
            return -1;
        }

        written += len;
    }

    return written;
}

int close(int fd) {
    auto fd_try = pop_file(fd);
    if (!fd_try) {
        USR_ERRNO = fd_try.error_code;
        return -1;
    }

    USR_ERRNO = fd_try.value->close();
    return USR_ERRNO ? -1 : 0;
}

int dup(int fd) {
    auto current = Proc::Process::current();
    auto fd_try  = get_file(fd);
    if (!fd_try) {
        USR_ERRNO = fd_try.error_code;
        return -1;
    }

    auto dup_try = fd_try.value->dup();
    if (!dup_try) {
        USR_ERRNO = dup_try.error_code;
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
        USR_ERRNO = fd_try.error_code;
        return -1;
    }

    auto dup_try = fd_try.value->dup();
    if (!dup_try) {
        USR_ERRNO = dup_try.error_code;
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
        USR_ERRNO = EINVAL;
        return -1;
    }

    int len = min<int>(strlen_try.value, sizeof(path_buffer) - 1);

    auto memcpy_try = u2k_memcpy(path_buffer, path, len);
    if (!memcpy_try) {
        USR_ERRNO = EINVAL;
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
        USR_ERRNO = ERANGE;
        return nullptr;
    }

    k2u_memcpy(buffer, cwd, len + 1);

    return buffer;
}

int statat(const usr_char* usr_path, stat* usr_statbuf, int flags) {
    char path_buffer[FS::MAX_PATH_LEN];

    if (!copy_and_canonize(path_buffer, usr_path)) {
        USR_ERRNO = EINVAL;
        return -1;
    }

    stat ker_statbuf;

    auto info = FS::File::info(path_buffer);
    if (!info) {
        USR_ERRNO = info.error_code;
        return -1;
    }

    ker_statbuf.st_rdev    = info.value.containing_dev_id;
    ker_statbuf.st_ino     = info.value.inode;
    ker_statbuf.st_mode    = info.value.mode | info.value.type;
    ker_statbuf.st_nlink   = info.value.hard_links;
    ker_statbuf.st_uid     = info.value.uid;
    ker_statbuf.st_gid     = info.value.gid;
    ker_statbuf.st_dev     = info.value.dev;
    ker_statbuf.st_size    = info.value.total_size;
    ker_statbuf.st_atime   = info.value.access_time;
    ker_statbuf.st_mtime   = info.value.modify_time;
    ker_statbuf.st_ctime   = info.value.change_time;
    ker_statbuf.st_blksize = info.value.block_size;
    ker_statbuf.st_blocks  = info.value.blocks;

    if (!u2k_memcpy(usr_statbuf, &ker_statbuf, sizeof(ker_statbuf))) {
        USR_ERRNO = info.error_code;
        return -1;
    }

    return 0;
}

int fstat(int fd, stat* usr_statbuf) {
    auto fd_try = get_file(fd);
    if (!fd_try) {
        USR_ERRNO = fd_try.error_code;
        return false;
    }

    stat ker_statbuf;

    auto info = fd_try.value->finfo();
    if (!info) {
        USR_ERRNO = info.error_code;
        return -1;
    }

    ker_statbuf.st_rdev    = info.value.containing_dev_id;
    ker_statbuf.st_ino     = info.value.inode;
    ker_statbuf.st_mode    = info.value.mode | info.value.type;
    ker_statbuf.st_nlink   = info.value.hard_links;
    ker_statbuf.st_uid     = info.value.uid;
    ker_statbuf.st_gid     = info.value.gid;
    ker_statbuf.st_dev     = info.value.dev;
    ker_statbuf.st_size    = info.value.total_size;
    ker_statbuf.st_atime   = info.value.access_time;
    ker_statbuf.st_mtime   = info.value.modify_time;
    ker_statbuf.st_ctime   = info.value.change_time;
    ker_statbuf.st_blksize = info.value.block_size;
    ker_statbuf.st_blocks  = info.value.blocks;

    if (!u2k_memcpy(usr_statbuf, &ker_statbuf, sizeof(ker_statbuf))) {
        USR_ERRNO = info.error_code;
        return -1;
    }

    return 0;
}

int access(const usr_char* usr_path, int mode) {
    char path_buffer[FS::MAX_PATH_LEN];

    if (!copy_and_canonize(path_buffer, usr_path)) {
        USR_ERRNO = EINVAL;
        return -1;
    }

    auto info = FS::File::info(path_buffer);
    if (!info) {
        USR_ERRNO = info.error_code;
        return -1;
    }

    // add permission checks

    return 0;
}

bool isatty(int fd) {
    auto fd_try = get_file(fd);
    if (!fd_try) {
        USR_ERRNO = fd_try.error_code;
        return false;
    }

    bool tty  = fd_try.value->isatty();
    USR_ERRNO = tty ? ENONE : ENOTTY;

    return tty;
}

void* mmap(void* addr, size_t length, int prot, int flags, int fd, FS::off_t offset) {
    auto fd_try = get_file(fd);
    if (!fd_try && !(flags & Mem::MAP_ANONYMOUS)) {
        USR_ERRNO = fd_try.error_code;
        return nullptr;
    }

    auto mmap_try =
        Mem::mmap(Proc::Process::current(), addr, length, prot, flags, fd_try.value, offset);
    if (!mmap_try) {
        USR_ERRNO = mmap_try.error_code;
        return nullptr;
    }

    return mmap_try.value;
}

int munmap(void* addr, size_t len) {
    auto result = Mem::munmap(Proc::Process::current(), addr, len);
    if (result != ENONE) {
        USR_ERRNO = result;
        return -1;
    }

    return 0;
}

long readdir(int fd, dirent* uent) {
    auto fd_try = get_file(fd);
    if (!fd_try) {
        USR_ERRNO = fd_try.error_code;
        return false;
    }

    auto rd_try = fd_try.value->readdir();
    if (!rd_try.has_value) {
        USR_ERRNO = rd_try.error_code;
        return -1;
    }

    dirent kent;

    kent.d_ino = rd_try.value.inode_id;
    strncpy(kent.d_name, rd_try.value.name, sizeof(kent.d_name));

    if (!u2k_memcpy(uent, &kent, sizeof(kent))) {
        USR_ERRNO = EINVAL;
        return -1;
    }

    return 0;
}

void seekdir(int fd, long pos) {
    auto fd_try = get_file(fd);
    if (!fd_try) {
        USR_ERRNO = fd_try.error_code;
        return;
    }

    USR_ERRNO = fd_try.value->seekdir(pos);
}

long telldir(int fd) {
    auto fd_try = get_file(fd);
    if (!fd_try) {
        USR_ERRNO = fd_try.error_code;
        return -1;
    }

    return fd_try.value->telldir();
}

void register_fs() {
    auto table = Sys::default_table();

    table[SYS_OPEN]    = (void*) open;
    table[SYS_READ]    = (void*) read;
    table[SYS_WRITE]   = (void*) write;
    table[SYS_CLOSE]   = (void*) close;
    table[SYS_FSTAT]   = (void*) fstat;
    table[SYS_ISATTY]  = (void*) isatty;
    table[SYS_DUP]     = (void*) dup;
    table[SYS_DUP2]    = (void*) dup2;
    table[SYS_CHDIR]   = (void*) chdir;
    table[SYS_GETCWD]  = (void*) getcwd;
    table[SYS_ACCESS]  = (void*) access;
    table[SYS_STATAT]  = (void*) statat;
    table[SYS_MMAP]    = (void*) mmap;
    table[SYS_MUNMAP]  = (void*) munmap;
    table[SYS_READDIR] = (void*) readdir;
    table[SYS_SEEKDIR] = (void*) seekdir;
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