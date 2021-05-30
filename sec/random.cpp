#include "aex/sec/random.hpp"

#include "aex/printk.hpp"

namespace AEX::Sec {
    uint32_t random_current = 0x6e796161;

    void feed_random(uint16_t val) {
        uint32_t proper = (val << 0) | (val << 16);
        random_current ^= proper * 563;
    }

    uint8_t random() {
        random_current = random_current * 1103511245 + 76555;
        return random_current;
    }
}