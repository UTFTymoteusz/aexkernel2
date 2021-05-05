#include "fs/devfile.hpp"

#include "aex/dev/chardevice.hpp"

#include "fs/file/charfile.hpp"

namespace AEX::FS {
    optional<File*> DevFile::open(Mem::SmartPointer<Dev::Device> dev, int mode) {
        switch (dev->type) {
        case Dev::DEV_BLOCK:
            return EBOTHER;
        case Dev::DEV_CHAR:
            return new CharFile(ENSURE_OPT(Dev::open_char_handle(dev->id, mode)));
        default:
            return ENOSYS;
        }
    }
}
