#pragma once

#include "aex/types.hpp"
#include "aex/utility.hpp"

namespace AEX::FS {
    class API Descriptor {
        public:
        File_SP file;
        int     flags;

        Descriptor() {}
        Descriptor(File_SP _file) : file(_file) {}
        Descriptor(File_SP _file, int _flags) : file(_file), flags(_flags) {}
    };
}