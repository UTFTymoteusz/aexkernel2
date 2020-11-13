#include "fs/devfile.hpp"

#include "aex/dev/chardevice.hpp"

#include "fs/devfiles/charfile.hpp"

namespace AEX::FS {
    optional<File*> DevFile::open(Mem::SmartPointer<Dev::Device> dev, int mode) {
        switch (dev->type) {
        case Dev::DEV_BLOCK:
            return EBOTHER;
        case Dev::DEV_CHAR: {
            auto chandle = Dev::open_char_handle(dev->id, mode);
            if (!chandle)
                return chandle.error_code;

            return new CharFile(chandle);
        }
        default:
            return ENOSYS;
        }
    }
}
