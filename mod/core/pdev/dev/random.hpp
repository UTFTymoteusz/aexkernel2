#include "aex/dev.hpp"
#include "aex/sec/random.hpp"

using namespace AEX;
using namespace AEX::Dev;

class Random : public CharDevice {
    public:
    Random() : CharDevice("random") {}

    error_t open(CharHandle*, int) {
        return ENONE;
    }

    error_t close(CharHandle*) {
        return ENONE;
    }

    optional<ssize_t> read(CharHandle*, void* ptr, size_t len) {
        uint8_t* bptr = (uint8_t*) ptr;

        for (size_t i = 0; i < len; i++)
            bptr[i] = Sec::random();
            
        return len;
    }

    optional<ssize_t> write(CharHandle*, const void*, size_t) {
        return ENOSPC;
    }
};