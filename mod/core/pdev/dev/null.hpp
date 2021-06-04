#include "aex/dev.hpp"

using namespace AEX;
using namespace AEX::Dev;

class Null : public CharDevice {
    public:
    Null() : CharDevice("null") {}

    error_t open(CharHandle*, int) {
        return ENONE;
    }

    error_t close(CharHandle*) {
        return ENONE;
    }

    optional<ssize_t> read(CharHandle*, void*, size_t) {
        return 0;
    }

    optional<ssize_t> write(CharHandle*, const void*, size_t len) {
        return len;
    }
};