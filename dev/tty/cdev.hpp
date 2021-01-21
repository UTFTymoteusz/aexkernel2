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

        optional<uint32_t> read(CharHandle* handle, void* ptr, uint32_t len);
        optional<uint32_t> write(CharHandle* handle, const void* ptr, uint32_t len);

        bool isatty();

        private:
        int       m_index;
        TTY::TTY* m_tty;

        Mem::Vector<CharHandle*> m_stack;
        CharHandle*              m_current;

        Mutex m_mutex;
        bool  m_closed;
    };
}