#include "aex/string.hpp"

#include "aex/math.hpp"
#include "aex/utility.hpp"

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

    void memset(void* mem, char c, size_t len) {
        char* m_mem = (char*) mem;

        for (size_t i = 0; i < len; i++)
            m_mem[i] = c;
    }

    void memset16(void* mem, uint16_t val, size_t count) {
        uint16_t* m_mem = (uint16_t*) mem;

        for (size_t i = 0; i < count; i++)
            m_mem[i] = val;
    }

    void memset32(void* mem, uint32_t val, size_t count) {
        uint32_t* m_mem = (uint32_t*) mem;

        for (size_t i = 0; i < count; i++)
            m_mem[i] = val;
    }

    void memset64(void* mem, uint64_t val, size_t count) {
        uint64_t* m_mem = (uint64_t*) mem;

        for (size_t i = 0; i < count; i++)
            m_mem[i] = val;
    }

    void memcpy(void* dst, const void* src, size_t size) {
        uint8_t* dst_b = (uint8_t*) dst;
        uint8_t* src_b = (uint8_t*) src;

        for (size_t i = 0; i < size; i++)
            dst_b[i] = src_b[i];
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

    char* strpivot(const char* str, size_t len) {
        return strncpy(new char[len + 1], str, len + 1);
    }
}