#pragma once

#include "aex/math.hpp"
#include "aex/utility.hpp"

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

namespace AEX {
    static const char* xtox_dictionary = "zyxwvutsrqponmlkjihgfedcba9876543210123456"
                                         "789abcdefghijklmnopqrstuvwxyz";

    API void snprintf(char* buffer, size_t n, const char* format, ...);
    API void snprintf(char* buffer, size_t n, const char* format, va_list args);

    template <typename T>
    API char* itos(T num, int base, char* buffer) {
        char* ptr;
        char* low;

        if (base < 2 || base > 36) {
            *buffer = '\0';
            return buffer;
        }

        ptr = buffer;
        if (num < 0 && base == 10)
            *ptr++ = '-';

        low = ptr;
        do {
            *ptr++ = xtox_dictionary[35 + num % base];
            num /= base;
        } while (num);

        *ptr-- = '\0';

        while (low < ptr) {
            char tmp = *low;

            *low++ = *ptr;
            *ptr-- = tmp;
        }
        return buffer;
    }

    template <typename T>
    API T stoi(int base, const char* str) {
        T   res  = 0;
        int sign = 1;
        int i    = 0;

        if (str[0] == '-') {
            sign = -1;
            ++i;
        }

        for (; str[i] != '\0'; ++i)
            res = res * base + str[i] - '0';

        return sign * res;
    }

    API size_t strlen(const char* str);
    API int    strcmp(const char* a, const char* b);
    API char*  strncpy(char* dst, const char* src, size_t num);

    API void memset(void* mem, char c, size_t len);
    API void memset16(void* mem, uint16_t val, size_t count);
    API void memset32(void* mem, uint32_t val, size_t count);
    API void memset64(void* mem, uint64_t val, size_t count);
    API void memcpy(void* dst, const void* src, size_t size);
    API int  memcmp(const void* a, const void* b, size_t num);

    API inline char tolower(char c) {
        return inrange(c, 'A', 'Z') ? c + 32 : c;
    }

    API inline char toupper(char c) {
        return inrange(c, 'a', 'z') ? c - 32 : c;
    }

    // mallocs(), copies the given string over into the allocated buffer and returns it.
    // automatically adds 1 to len to compensate for the null byte at the end.
    API char* strpivot(const char* str, size_t len);
}