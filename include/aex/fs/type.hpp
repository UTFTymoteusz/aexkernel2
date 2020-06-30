#pragma once

namespace AEX::FS {
    enum fs_type_t {
        UNKNOWN   = 0x0000,
        REGULAR   = 0x0001,
        DIRECTORY = 0x0010,
        CHAR      = 0x0002,
        BLOCK     = 0x0006,
        NET       = 0x000A,
    };
}