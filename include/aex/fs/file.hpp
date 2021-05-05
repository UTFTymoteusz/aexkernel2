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
    enum file_mode_t {
        O_RD        = 0x01,
        O_WR        = 0x02,
        O_APPEND    = 0x04,
        O_DIRECTORY = 0x08,
        O_RDWR      = O_RD | O_WR,
    };

    enum at_t {
        AT_NONE             = 0x00,
        AT_EMPTY_PATH       = 0x01,
        AT_SYMLINK_NOFOLLOW = 0x02,
    };

    enum fd_t {
        FD_CLOEXEC = 0x0001,
    };

    enum o_t {
        O_NONBLOCK = 0x0001,
    };

    class API File {
        public:
        enum seek_mode {
            SEEK_SET     = 0,
            SEEK_CURRENT = 1,
            SEEK_END     = 2,
        };

        virtual ~File();

        static optional<File_SP>    open(const char* path, int mode);
        virtual optional<ssize_t>   read(void* buf, size_t count);
        virtual optional<ssize_t>   write(const void* buf, size_t count);
        virtual error_t             close();
        virtual optional<off_t>     seek(off_t offset, seek_mode mode = seek_mode::SEEK_SET);
        static optional<file_info>  info(const char* path, int flags = 0);
        virtual optional<file_info> finfo();
        virtual error_t             fchmod(mode_t mode);
        virtual optional<File_SP>   dup();

        virtual optional<Mem::Region*> mmap(Proc::Process* process, void*, size_t len, int flags,
                                            FS::File_SP file, FS::off_t offset);
        virtual optional<int>          ioctl(int rq, uint64_t val);
        virtual bool                   isatty();

        virtual optional<dirent> readdir();
        virtual error_t          seekdir(long pos);
        virtual long             telldir();

        int  get_flags();
        void set_flags(int);

        protected:
        int m_flags;
    };
}