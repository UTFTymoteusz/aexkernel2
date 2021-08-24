#include "aex/string.hpp"

#include "aex/math.hpp"
#include "aex/utility.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX {
    size_t strlen(const char* str) {
        size_t len = 0;

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

    size_t strlcpy(char* dst, const char* src, size_t num) {
        size_t src_len = strlen(src);

        if (num == 0)
            return src_len;

        size_t len = min(src_len, num - 1);
        memcpy(dst, src, len);

        dst[len] = '\0';

        return src_len;
    }

    char* strndup(const char* str, size_t len) {
        char* nstr = new char[len + 1];
        strlcpy(nstr, str, len + 1);

        return nstr;
    }

    size_t strspn(const char* str, const char* bongs) {
        size_t n    = 0;
        size_t cmps = strlen(bongs);

        while (str[n] != '\0') {
            bool safe = false;

            for (size_t i = 0; i < cmps; i++)
                if (str[n] == bongs[i]) {
                    safe = true;
                    break;
                }

            if (!safe)
                break;

            n++;
        }

        return n;
    }

    size_t strcspn(const char* str, const char* bongs) {
        size_t n    = 0;
        size_t cmps = strlen(bongs);

        while (str[n] != '\0') {
            for (size_t i = 0; i < cmps; i++)
                if (str[n] == bongs[i])
                    return n;

            n++;
        }

        return n;
    }

    char* strntokp_r(char* buffer, size_t len, const char* str, const char* delim,
                     const char** saveptr) {
        if (str) {
            size_t len = strspn(str, delim);
            if (len == 0)
                if (*str == '\0')
                    return NULL;

            *saveptr = str + len;
        }

        size_t tok_len = strcspn(*saveptr, delim);
        if (tok_len == 0)
            return nullptr;

        size_t copy_len = min(len, tok_len);

        if (copy_len < len)
            buffer[copy_len] = '\0';

        memcpy(buffer, *saveptr, copy_len);
        *saveptr += tok_len;

        size_t delim_len = strspn(*saveptr, delim);
        *saveptr += delim_len;

        return buffer;
    }

    char* strchr(const char* str, int c) {
        size_t len = strlen(str);

        for (size_t i = 0; i < len; i++)
            if (str[i] == c)
                return (char*) &str[i];

        return nullptr;
    }

    char* strrchr(const char* str, int c) {
        size_t len = strlen(str);

        for (size_t i = len - 1; i > 0; i--)
            if (str[i] == c)
                return (char*) &str[i];

        return nullptr;
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
}