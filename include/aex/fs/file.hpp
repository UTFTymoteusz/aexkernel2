#pragma once

#include "aex/dev/device.hpp"
#include "aex/fs/directory.hpp"
#include "aex/fs/types.hpp"
#include "aex/mem/smartptr.hpp"
#include "aex/optional.hpp"
#include "aex/types.hpp"
#include "aex/utility.hpp"

namespace AEX::Mem {
    class Region;
}

namespace AEX::FS {
    enum o_flags_t {
        O_RDONLY    = 0x01,
        O_WRONLY    = 0x02,
        O_APPEND    = 0x04,
        O_DIRECTORY = 0x08,
        O_NONBLOCK  = 0x10,
        O_CREAT     = 0x20,
        O_EXCL      = 0x40,
        O_ACCMODE   = 0xFFFF0000,
        O_RDWR      = O_RDONLY | O_WRONLY,
    };

    enum at_t {
        AT_NONE             = 0x00,
        AT_EMPTY_PATH       = 0x01,
        AT_SYMLINK_NOFOLLOW = 0x02,
    };

    enum fd_t {
        FD_CLOEXEC = 0x0001,
    };

    struct find_result;

    class API File {
        public:
        enum seek_mode {
            SEEK_SET     = 0,
            SEEK_CURRENT = 1,
            SEEK_END     = 2,
        };

        virtual ~File();

        static optional<File_SP>   open(const char* path, int flags);
        static optional<File_SP>   mkdir(const char* path, int flags);
        static error_t             rename(const char* oldpath, const char* newpath);
        static error_t             unlink(const char* path);
        static error_t             rmdir(const char* path);
        static optional<file_info> info(const char* path, int flags = 0);

        virtual optional<ssize_t>   read(void* buf, size_t count);
        virtual optional<ssize_t>   write(const void* buf, size_t count);
        virtual error_t             close();
        virtual optional<off_t>     seek(off_t offset, seek_mode mode = seek_mode::SEEK_SET);
        virtual optional<file_info> finfo();
        virtual error_t             fchmod(mode_t mode);

        virtual optional<Mem::Region*> mmap(Proc::Process* process, void*, size_t len, int flags,
                                            FS::File_SP file, FS::off_t offset);
        virtual optional<int>          ioctl(int rq, uint64_t val);
        virtual bool                   isatty();

        virtual optional<dirent> readdir();
        virtual error_t          seekdir(long pos);
        virtual long             telldir();

        int  getFlags();
        void setFlags(int);

        protected:
        int m_flags;

        private:
        static optional<find_result> get(const char* path, bool allow_incomplete = false);
    };
}