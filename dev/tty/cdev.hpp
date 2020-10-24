#pragma once

#include "aex/dev/chardevice.hpp"
#include "aex/mem/vector.hpp"
#include "aex/mutex.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Dev {
    namespace TTY {
        class VTTY;
    }

    class TTYChar : public CharDevice {
        public:
        TTYChar(int index, const char* name);

        error_t open(CharHandle* handle);
        error_t close(CharHandle* handle);

        optional<uint32_t> read(CharHandle* handle, void* ptr, uint32_t len);
        optional<uint32_t> write(CharHandle* handle, const void* ptr, uint32_t len);

        private:
        int        m_index;
        TTY::VTTY* m_vtty;

        Mem::Vector<CharHandle*> m_stack;
        CharHandle*              m_current;

        Mutex m_mutex;
    };
}