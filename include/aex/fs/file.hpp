#pragma once

#include "aex/dev/device.hpp"
#include "aex/fs/directory.hpp"
#include "aex/fs/types.hpp"
#include "aex/mem/smartptr.hpp"
#include "aex/optional.hpp"
#include "aex/types.hpp"

namespace AEX::FS {
    enum file_mode_t {
        O_RD   = 0x01,
        O_WR   = 0x02,
        O_RDWR = O_RD | O_WR,
    };

    enum at_t {
        AT_NONE             = 0x00,
        AT_EMPTY_PATH       = 0x01,
        AT_SYMLINK_NOFOLLOW = 0x02,
    };

    enum fd_t {
        FD_CLOEXEC = 0x0001,
    };

    class File {
        public:
        enum seek_mode {
            SEEK_SET     = 0,
            SEEK_CURRENT = 1,
            SEEK_END     = 2,
        };

        virtual ~File();

        static optional<File_SP> open(const char* path, int mode);

        static optional<file_info> info(const char* path, int flags = 0);

        virtual optional<ssize_t> read(void* buf, size_t count);
        virtual optional<ssize_t> write(void* buf, size_t count);

        virtual optional<file_info> finfo();
        virtual error_t             fchmod(mode_t mode);

        virtual optional<off_t> seek(off_t offset, seek_mode mode = seek_mode::SEEK_SET);

        virtual optional<dir_entry> readdir();
        virtual error_t             seekdir(long pos);
        virtual long                telldir();

        virtual optional<File_SP> dup();

        virtual error_t close();

        virtual bool isatty();

        int  get_flags();
        void set_flags(int);

        protected:
        int m_flags;
    };
}