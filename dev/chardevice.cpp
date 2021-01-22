#include "aex/dev/chardevice.hpp"

#include "aex/dev.hpp"
#include "aex/types.hpp"

namespace AEX::Dev {
    CharDevice::CharDevice(const char* name) : Device(name, DEV_CHAR) {
        //
    }

    CharDevice::~CharDevice() {
        //
    }

    error_t CharDevice::open(CharHandle*, int) {
        return ENONE;
    }

    error_t CharDevice::close(CharHandle*) {
        return ENONE;
    }

    optional<ssize_t> CharDevice::read(CharHandle*, void*, size_t) {
        return ENOSYS;
    }

    optional<ssize_t> CharDevice::write(CharHandle*, const void*, size_t) {
        return ENOSYS;
    }

    bool CharDevice::isatty() {
        return false;
    }

    optional<CharHandle_SP> open_char_handle(int id, int mode) {
        auto device = devices.get(id);
        if (!device || device->type != DEV_CHAR)
            return {};

        auto chr_device = (Dev::CharDevice_SP) device;
        auto handle     = new CharHandle(chr_device);
        auto error      = chr_device->open(handle, mode);
        if (error) {
            delete handle;
            return error;
        }

        return CharHandle_SP(handle);
    }
}