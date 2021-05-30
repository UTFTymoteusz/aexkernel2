#include "aex/dev.hpp"

using namespace AEX;
using namespace AEX::Dev;

class Zero : public CharDevice {
    public:
    Zero() : CharDevice("zero") {}

    error_t open(CharHandle*, int) {
        return ENONE;
    }

    error_t close(CharHandle*) {
        return ENONE;
    }

    optional<ssize_t> read(CharHandle*, void* ptr, size_t len) {
        memset(ptr, '\0', len);
        return len;
    }

    optional<ssize_t> write(CharHandle*, const void*, size_t len) {
        return len;
    }
};