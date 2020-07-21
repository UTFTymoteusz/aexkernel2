#pragma once

namespace AEX::FS {
    enum fs_type_t {
        FILE_UNKNOWN   = 0x0000,
        FILE_REGULAR   = 0x0001,
        FILE_DIRECTORY = 0x0010,
        FILE_CHAR      = 0x0002,
        FILE_BLOCK     = 0x0006,
        FILE_NET       = 0x000A,
    };
}