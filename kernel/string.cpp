#include "aex/string.hpp"

#include "aex/math.hpp"
#include "aex/utility.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX {
    int WEAK strlen(const char* str) {
        int len = 0;

        while (*str++ != '\0')
            len++;

        return len;
    }

    int WEAK strcmp(const char* a, const char* b) {
        do {
            if (*a < *b)
                return -1;
            else if (*a > *b)
                return 1;
        } while (*a++ != '\0' && *b++ != '\0');

        return 0;
    }

    char* WEAK strncpy(char* dst, const char* src, size_t num) {
        int len = min((size_t) strlen(src), num - 1);
        if (len < 0)
            return dst;

        for (int i = 0; i < len; i++)
            dst[i] = src[i];

        dst[len] = '\0';

        return dst;
    }

    void WEAK memset(void* mem, char c, size_t len) {
        char* m_mem = (char*) mem;

        for (size_t i = 0; i < len; i++)
            m_mem[i] = c;
    }

    void WEAK memset16(void* mem, uint16_t n, size_t count) {
        uint16_t* m_mem = (uint16_t*) mem;

        for (size_t i = 0; i < count; i++)
            m_mem[i] = n;
    }

    void WEAK memset32(void* mem, uint32_t n, size_t count) {
        uint32_t* m_mem = (uint32_t*) mem;

        for (size_t i = 0; i < count; i++)
            m_mem[i] = n;
    }

    void WEAK memset64(void* mem, uint64_t n, size_t count) {
        uint64_t* m_mem = (uint64_t*) mem;

        for (size_t i = 0; i < count; i++)
            m_mem[i] = n;
    }

    void WEAK memcpy(void* dst, const void* src, size_t size) {
        uint8_t* dst_b = (uint8_t*) dst;
        uint8_t* src_b = (uint8_t*) src;

        for (size_t i = 0; i < size; i++)
            dst_b[i] = src_b[i];

        /*size_t aligned = size / sizeof(size_t);

        size_t* dst_st = (size_t*) dst;
        size_t* src_st = (size_t*) src;

        for (size_t i = 0; i < aligned; i++)
            dst_st[i] = src_st[i];

        size_t remainder = size - aligned * sizeof(size_t);

        uint8_t* dst_b = (uint8_t*) dst + aligned * sizeof(size_t);
        uint8_t* src_b = (uint8_t*) src + aligned * sizeof(size_t);

        for (size_t i = 0; i < remainder; i++)
            dst_b[i] = src_b[i];*/
    }

    int WEAK memcmp(const void* a, const void* b, size_t num) {
        uint8_t* _a = (uint8_t*) a;
        uint8_t* _b = (uint8_t*) b;

        for (size_t i = 0; i < num; i++)
            if (_a[i] < _b[i])
                return -1;
            else if (_a[i] > _b[i])
                return 1;

        return 0;
    }
}