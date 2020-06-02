#include "aex/string.hpp"

#include "aex/math.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX {
    int strlen(const char* str) {
        int len = 0;

        while (*str++ != '\0')
            len++;

        return len;
    }

    int strcmp(const char* a, const char* b) {
        do {
            if (*a < *b)
                return -1;
            else if (*a > *b)
                return 1;
        } while (*a++ != '\0' && *b++ != '\0');

        return 0;
    }

    char* strncpy(char* dst, const char* src, size_t num) {
        int len = min((size_t) strlen(src), num - 1);
        if (len < 0)
            return dst;

        for (int i = 0; i < len; i++)
            dst[i] = src[i];

        dst[len] = '\0';

        return dst;
    }

    void __attribute((weak)) memset(void* mem, char c, size_t len) {
        char* _mem = (char*) mem;

        for (size_t i = 0; i < len; i++)
            _mem[i] = c;
    }

    void __attribute((weak)) memset16(void* mem, uint16_t n, size_t count) {
        uint16_t* _mem = (uint16_t*) mem;

        for (size_t i = 0; i < count; i++)
            _mem[i] = n;
    }

    void __attribute((weak)) memset32(void* mem, uint32_t n, size_t count) {
        uint32_t* _mem = (uint32_t*) mem;

        for (size_t i = 0; i < count; i++)
            _mem[i] = n;
    }

    void __attribute((weak)) memset64(void* mem, uint64_t n, size_t count) {
        uint64_t* _mem = (uint64_t*) mem;

        for (size_t i = 0; i < count; i++)
            _mem[i] = n;
    }

    void __attribute((weak)) memcpy(void* dst, const void* src, size_t size) {
        size_t aligned = size / 8;

        uint64_t* dst64 = (uint64_t*) dst;
        uint64_t* src64 = (uint64_t*) src;

        for (size_t i = 0; i < aligned; i++)
            dst64[i] = src64[i];

        size_t remainder = size - aligned * 8;

        uint8_t* dst8 = (uint8_t*) dst + aligned * 8;
        uint8_t* src8 = (uint8_t*) src + aligned * 8;

        for (size_t i = 0; i < remainder; i++)
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