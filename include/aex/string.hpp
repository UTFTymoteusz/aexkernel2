#pragma once

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

namespace AEX {
    static const char* dictionary = "zyxwvutsrqponmlkjihgfedcba9876543210123456"
                                    "789abcdefghijklmnopqrstuvwxyz";

    void snprintf(char* buffer, size_t n, const char* format, ...);
    void snprintf(char* buffer, size_t n, const char* format, va_list args);

    template <typename T>
    char* itos(T num, int base, char* buffer) {
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
            *ptr++ = dictionary[35 + num % base];
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
    T stoi(int base, const char* str) {
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

    int   strlen(const char* str);
    int   strcmp(const char* a, const char* b);
    char* strncpy(char* dst, const char* src, size_t num);

    void memset(void* mem, char c, size_t len);
    void memset32(void* mem, uint32_t n, size_t count);
    void memset64(void* mem, uint64_t n, size_t count);
    void memcpy(void* dst, const void* src, size_t size);
    int  memcmp(const void* a, const void* b, size_t num);
} // namespace AEX::String