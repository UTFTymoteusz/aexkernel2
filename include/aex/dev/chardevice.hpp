#pragma once

#include "aex/dev/charhandle.hpp"
#include "aex/dev/device.hpp"
#include "aex/errno.hpp"
#include "aex/mem.hpp"
#include "aex/optional.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Dev {
    class CharDevice : public Device {
        public:
        CharDevice(const char* name);

        virtual ~CharDevice();

        virtual error_t open(CharHandle* handle, int mode);
        virtual error_t close(CharHandle* handle);

        virtual optional<uint32_t> read(CharHandle* handle, void* ptr, uint32_t len);
        virtual optional<uint32_t> write(CharHandle* handle, const void* ptr, uint32_t len);
    };

    typedef Mem::SmartPointer<CharDevice> CharDevice_SP;

    optional<CharHandle_SP> open_char_handle(int id, int mode);
}