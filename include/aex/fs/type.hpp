#pragma once

namespace AEX::FS {
    enum fs_type_t {
        FT_UNKNOWN   = 0x0000,
        FT_REGULAR   = 0x0001,
        FT_DIRECTORY = 0x0010,
        FT_CHAR      = 0x0002,
        FT_BLOCK     = 0x0006,
        FT_NET       = 0x000A,
    };
}