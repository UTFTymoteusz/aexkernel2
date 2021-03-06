#pragma once

#include "aex/dev/chardevice.hpp"
#include "aex/mem/vector.hpp"
#include "aex/mutex.hpp"
#include "aex/types.hpp"

namespace AEX::Dev {
    namespace TTY {
        class TTY;
    }

    class TTYChar : public CharDevice {
        public:
        TTYChar(int index, const char* name);

        error_t open(CharHandle* handle, int mode);
        error_t close(CharHandle* handle);

        optional<ssize_t>          read(CharHandle* handle, void* ptr, size_t len);
        optional<ssize_t>          write(CharHandle* handle, const void* ptr, size_t len);
        optional<int>              ioctl(CharHandle* handle, int rq, uint64_t val);
        optional<Mem::MMapRegion*> mmap(Proc::Process*, void*, size_t, int, FS::File_SP, FS::off_t);
        bool                       isatty();

        private:
        int       m_index;
        TTY::TTY* m_tty;

        Mem::Vector<CharHandle*> m_stack;
        CharHandle*              m_current;

        Mutex m_mutex;
        bool  m_closed;
    };
}