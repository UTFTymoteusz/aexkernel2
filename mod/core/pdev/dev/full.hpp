#include "aex/dev.hpp"

using namespace AEX;
using namespace AEX::Dev;

class Full : public CharDevice {
    public:
    Full() : CharDevice("full") {}

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

    optional<ssize_t> write(CharHandle*, const void*, size_t) {
        return ENOSPC;
    }
};