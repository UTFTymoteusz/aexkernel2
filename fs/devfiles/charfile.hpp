#pragma once

#include "aex/dev/charhandle.hpp"
#include "aex/fs/file.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::FS {
    class CharFile : public File {
        public:
        CharFile(Dev::CharHandle_SP handle);
        ~CharFile();

        optional<uint32_t> read(void* buf, uint32_t count);
        optional<uint32_t> write(void* buf, uint32_t count);

        private:
        Dev::CharHandle_SP m_handle;
    };
}
