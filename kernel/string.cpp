#include "kernel/string.hpp"

#include "kernel/printk.hpp"

#include <stdint.h>

namespace AEX {
    int strlen(char* str) {
        int len = 0;

        while (*str++ != '\0')
            len++;

        return len;
    }

    void memset(void* mem, char c, size_t len) {
        char* _mem = (char*) mem;

        for (size_t i = 0; i < len; i++)
            _mem[i] = c;
    }

    void memcpy(void* dst, void* src, size_t size) {
        /*size_t aligned = size / 8;

        uint64_t* dst64 = (uint64_t*) dst;
        uint64_t* src64 = (uint64_t*) src;

        for (size_t i = 0; i < aligned; i++)
            dst64[i] = src64[i];

        size_t remainder = size - aligned * 8;

        uint8_t* dst8 = (uint8_t*) dst + aligned * 8;
        uint8_t* src8 = (uint8_t*) src + aligned * 8;

        for (size_t i = 0; i < remainder; i++)
            dst8[i] = src8[i];*/

        uint8_t* dst8 = (uint8_t*) dst;
        uint8_t* src8 = (uint8_t*) src;

        for (size_t i = 0; i < size; i++)
            dst8[i] = src8[i];
    }

    int memcmp(const void* a, const void* b, size_t num) {
        uint8_t* _a = (uint8_t*) a;
        uint8_t* _b = (uint8_t*) b;

        for (size_t i = 0; i < num; i++)
            if (_a[i] < _b[i])
                return -1;
            else if (_a[i] > _b[i])
                return 1;

        return 0;
    }
} // namespace AEX::String