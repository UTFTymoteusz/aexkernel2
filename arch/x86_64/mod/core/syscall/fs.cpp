#include "aex/errno.hpp"
#include "aex/fs/file.hpp"
#include "aex/proc/process.hpp"
#include "aex/sys/syscall.hpp"
#include "aex/types.hpp"

#include "fcntl.h"
#include "syscallids.h"
#include "types.hpp"
#include "usr.hpp"

using namespace AEX;
using namespace AEX::Proc;

optional<FS::File_SP> get_file(int fd);
optional<FS::File_SP> pop_file(int fd);
void                  set_flags(int fd, int flags);
optional<int>         get_flags(int fd);

bool copy_and_canonize(char* buffer, const usr_char* usr_path);

int open(const usr_char* usr_path, int mode) {
    auto current = Proc::Process::current();
    char path_buffer[FS::MAX_PATH_LEN];

    USR_ENSURE(copy_and_canonize(path_buffer, usr_path));

    auto file = USR_ENSURE_OPT(FS::File::open(path_buffer, mode));

    current->descs_lock.acquire();
    int fd = current->descs.push(file);
    current->descs_lock.release();

    return fd;
}

ssize_t read(int fd, usr_void* usr_buf, size_t count) {
    auto file      = USR_ENSURE_OPT(get_file(fd));
    auto usr_buf_c = (usr_uint8_t*) usr_buf;

    uint8_t buffer[1024];
    ssize_t read = 0;

    for (size_t i = 0; i < count; i += sizeof(buffer)) {
        int  len      = min<size_t>(count - i, sizeof(buffer));
        auto read_try = file->read(buffer, len);

        USR_ENSURE(read_try);
        USR_ENSURE(k2u_memcpy(&usr_buf_c[i], buffer, len));

        read += read_try.value;
        if (read_try.error_code != EINTR)
            break;
    }

    return read;
}

ssize_t write(int fd, const usr_void* usr_buf, size_t count) {
    auto file      = USR_ENSURE_OPT(get_file(fd));
    auto usr_buf_c = (const usr_uint8_t*) usr_buf;

    uint8_t buffer[1024];
    ssize_t written = 0;

    for (size_t i = 0; i < count; i += sizeof(buffer)) {
        int len = min<size_t>(count - i, sizeof(buffer));
        USR_ENSURE(u2k_memcpy(buffer, &usr_buf_c[i], len));

        written += USR_ENSURE_OPT(file->write(buffer, len));
    }

    return written;
}

int close(int fd) {
    return USR_ENSURE_ENONE(USR_ENSURE_OPT(get_file(fd))->close());
}

int ioctl(int fd, int rq, uint64_t val) {
    return USR_ENSURE_OPT(USR_ENSURE_OPT(get_file(fd))->ioctl(rq, val));
}

int dup(int fd) {
    auto current = Proc::Process::current();
    auto file    = USR_ENSURE_OPT(get_file(fd));
    auto dupd    = USR_ENSURE_OPT(file->dup());

    current->descs_lock.acquire();
    int fd2 = current->descs.push(dupd);
    current->descs_lock.release();

    return fd2;
}

int dup2(int srcfd, int dstfd) {
    auto current = Proc::Process::current();
    auto file    = USR_ENSURE_OPT(get_file(srcfd));
    auto dupd    = USR_ENSURE_OPT(file->dup());

    current->descs_lock.acquire();
    current->descs.set(dstfd, dupd);
    current->descs_lock.release();

    return dstfd;
}

// this needs path verification
int chdir(const usr_char* path) {
    char path_buffer[FS::MAX_PATH_LEN];

    int strlen = USR_ENSURE_OPT(usr_strlen(path));
    int len    = min<int>(strlen, sizeof(path_buffer) - 1);

    USR_ENSURE_OPT(u2k_memcpy(path_buffer, path, len));
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
    USR_ENSURE(copy_and_canonize(path_buffer, usr_path));

    stat ker_statbuf;
    auto info = USR_ENSURE(FS::File::info(path_buffer));

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

    USR_ENSURE_OPT(u2k_memcpy(usr_statbuf, &ker_statbuf, sizeof(ker_statbuf)));
    return 0;
}

int fstat(int fd, stat* usr_statbuf) {
    auto file = USR_ENSURE_OPT(get_file(fd));
    auto info = USR_ENSURE_OPT(file->finfo());

    stat ker_statbuf;

    ker_statbuf.st_rdev    = info.containing_dev_id;
    ker_statbuf.st_ino     = info.inode;
    ker_statbuf.st_mode    = info.mode | info.type;
    ker_statbuf.st_nlink   = info.hard_links;
    ker_statbuf.st_uid     = info.uid;
    ker_statbuf.st_gid     = info.gid;
    ker_statbuf.st_dev     = info.dev;
    ker_statbuf.st_size    = info.total_size;
    ker_statbuf.st_atime   = info.access_time;
    ker_statbuf.st_mtime   = info.modify_time;
    ker_statbuf.st_ctime   = info.change_time;
    ker_statbuf.st_blksize = info.block_size;
    ker_statbuf.st_blocks  = info.blocks;

    USR_ENSURE_OPT(u2k_memcpy(usr_statbuf, &ker_statbuf, sizeof(ker_statbuf)));
    return 0;
}

int access(const usr_char* usr_path, int mode) {
    char path_buffer[FS::MAX_PATH_LEN];

    USR_ENSURE(copy_and_canonize(path_buffer, usr_path));
    USR_ENSURE_OPT(FS::File::info(path_buffer));

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
    printk("pid%i: mmap(0x%p, %i, %i, %i, %i, %i)\n", Proc::Process::current()->pid, addr, length,
           prot, flags, fd, offset);

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
    auto file   = USR_ENSURE_OPT(get_file(fd));
    auto dentry = USR_ENSURE_OPT(file->readdir());

    dirent kent;

    kent.d_ino = dentry.inode_id;
    strncpy(kent.d_name, dentry.name, sizeof(kent.d_name));

    USR_ENSURE_OPT(u2k_memcpy(uent, &kent, sizeof(kent)));
    return 0;
}

long seek(int fd, long pos, int mode) {
    auto fd_try = get_file(fd);
    if (!fd_try) {
        USR_ERRNO = fd_try.error_code;
        return -1;
    }

    USR_ENSURE(mode > FS::File::SEEK_SET && mode < FS::File::SEEK_END);
    return USR_ENSURE_OPT(fd_try.value->seek(pos, (FS::File::seek_mode) mode));
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
    auto file = USR_ENSURE_OPT(get_file(fd));
    return file->telldir();
}

int fcntl(int fd, int cmd, int val) {
    auto file  = USR_ENSURE_OPT(get_file(fd));
    auto flags = USR_ENSURE_OPT(get_flags(fd));

    switch (cmd) {
    case F_DUPFD:
        return dup(fd);
    case F_GETFL:
        return file->get_flags() & 0xFFFF | flags << 16;
    case F_SETFL:
        USR_ENSURE_FL(val, 0x00010001);

        set_flags(fd, val >> 16);
        file->set_flags(file->get_flags() | (val & 0xFFFF));

        return 0;
    default:
        return -1;
    }
}

void register_fs() {
    auto table = Sys::default_table();

    table[SYS_OPEN]    = (void*) open;
    table[SYS_READ]    = (void*) read;
    table[SYS_WRITE]   = (void*) write;
    table[SYS_CLOSE]   = (void*) close;
    table[SYS_SEEK]    = (void*) seek;
    table[SYS_IOCTL]   = (void*) ioctl;
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
    table[SYS_FCNTL]   = (void*) fcntl;
}

optional<FS::File_SP> get_file(int fd) {
    auto current = Proc::Process::current();
    auto scope   = current->descs_lock.scope();

    if (!current->descs.present(fd)) {
        // PRINTK_DEBUG_WARN1("ebadf (%i)", fd);
        return EBADF;
    }

    return current->descs[fd].file;
}

optional<FS::File_SP> pop_file(int fd) {
    auto current = Proc::Process::current();
    auto scope   = current->descs_lock.scope();

    if (!current->descs.present(fd)) {
        // PRINTK_DEBUG_WARN1("ebadf (%i)", fd);
        return EBADF;
    }

    auto desc = current->descs[fd];
    current->descs.erase(fd);

    return desc.file;
}

optional<int> get_flags(int fd) {
    auto current = Proc::Process::current();
    auto scope   = current->descs_lock.scope();

    if (!current->descs.present(fd)) {
        // PRINTK_DEBUG_WARN1("ebadf (%i)", fd);
        return EBADF;
    }

    return current->descs[fd].flags;
}

void set_flags(int fd, int flags) {
    auto current = Proc::Process::current();
    auto scope   = current->descs_lock.scope();

    if (!current->descs.present(fd))
        return;

    auto desc  = current->descs[fd];
    desc.flags = flags;

    current->descs.set(fd, desc);
}