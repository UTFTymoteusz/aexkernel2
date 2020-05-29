#pragma once

namespace AEX {
    enum amd64_rel_type {
        R_AMD64_NONE  = 0,
        R_AMD64_64    = 1,
        R_AMD64_PC32  = 2,
        R_AMD64_GOT32 = 3,
        R_AMD64_PLT32 = 4,
        R_AMD64_32S   = 11,
    };
};