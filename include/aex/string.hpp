#pragma once

#include <stddef.h>

namespace AEX {
    static const char* dictionary = "zyxwvutsrqponmlkjihgfedcba9876543210123456"
                                    "789abcdefghijklmnopqrstuvwxyz";

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
    char* strncpy(char* dst, const char* src, size_t num);

    void memset(void* mem, char c, size_t len);
    void memcpy(void* dst, const void* src, size_t size);
    int  memcmp(const void* a, const void* b, size_t num);
} // namespace AEX::String