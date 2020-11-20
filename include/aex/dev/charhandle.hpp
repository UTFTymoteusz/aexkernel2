#pragma once

#include "aex/mem/smartptr.hpp"
#include "aex/mutex.hpp"
#include "aex/optional.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Dev {
    class CharDevice;

    class CharHandle {
        public:
        void* device_data = nullptr;

        CharHandle();
        CharHandle(Mem::SmartPointer<CharDevice> chrhndl);
        ~CharHandle();

        optional<uint32_t> read(void* ptr, uint32_t len);
        optional<uint32_t> write(const void* ptr, uint32_t len);

        bool isatty();

        private:
        Mem::SmartPointer<CharDevice> m_dev;
    };

    typedef Mem::SmartPointer<CharHandle> CharHandle_SP;
}
