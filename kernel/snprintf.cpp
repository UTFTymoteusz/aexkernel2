#include "aex/math.hpp"
#include "aex/string.hpp"
#include "aex/utility.hpp"

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

namespace AEX {
    enum size {
        SIZE_SHORT = 0,
        SIZE_INT   = 1,
        SIZE_LONG  = 2,
    };

    void snprintf(char* buffer, size_t len, const char* format, ...) {
        va_list args;
        va_start(args, format);

        snprintf(buffer, len, format, args);

        va_end(args);
    }

    void snprintf(char* buffer, size_t len, const char* format, va_list args) {
        if (len == 0)
            return;

        len--;

        auto snprintf_common = [&buffer, &len](char padchar, int padlen, const char* str) {
            if (len == 0)
                return;

            if (!str)
                str = "<null>";

            size_t pad_len = min((size_t) max<int>(padlen - strlen(str), 0), len);
            len -= pad_len;

            for (size_t i = 0; i < pad_len; i++) {
                *buffer = padchar;
                buffer++;
            }

            if (len == 0)
                return;

            size_t total_len = min(strlen(str), len);
            if (total_len == 0)
                return;

            memcpy(buffer, str, total_len);

            buffer += total_len;
            len -= total_len;
        };

        char tmp_buffer[72];

        do {
            char c = *format;

            if (c == '%') {
                int  padlen  = 0;
                char padchar = ' ';

                c        = *++format;
                int size = SIZE_INT;

                if (c == '0') {
                    padchar = c;
                    c       = *++format;
                }

                int ptr = 0;
                while (c >= '0' && c <= '9') {
                    tmp_buffer[ptr++] = c;

                    c = *++format;
                }
                tmp_buffer[ptr] = '\0';

                if (ptr != 0)
                    padlen = stoi<int>(10, tmp_buffer);

                switch (c) {
                case 'l':
                    size = SIZE_LONG;
                    ++format;
                    break;
                case 'h':
                    size = SIZE_SHORT;
                    ++format;
                    break;
                }
                c = *format;

                switch (c) {
                case '\0':
                    break;
                case 's':
                    snprintf_common(padchar, padlen, va_arg(args, char*));
                    break;
                case 'c':
                    if (len == 0)
                        continue;

                    tmp_buffer[0] = (char) va_arg(args, int);
                    tmp_buffer[1] = '\0';

                    snprintf_common(padchar, padlen, tmp_buffer);
                    break;
                case 'i':
                    if (size == SIZE_SHORT)
                        itos((int16_t) va_arg(args, int), 10, tmp_buffer);
                    else if (size == SIZE_LONG)
                        itos((int64_t) va_arg(args, long), 10, tmp_buffer);
                    else
                        itos((int32_t) va_arg(args, int), 10, tmp_buffer);

                    snprintf_common(padchar, padlen, tmp_buffer);
                    break;
                case 'u':
                    if (size == SIZE_SHORT)
                        itos((uint16_t) va_arg(args, unsigned int), 10, tmp_buffer);
                    else if (size == SIZE_LONG)
                        itos((uint64_t) va_arg(args, unsigned long), 10, tmp_buffer);
                    else
                        itos((uint32_t) va_arg(args, unsigned int), 10, tmp_buffer);

                    snprintf_common(padchar, padlen, tmp_buffer);
                    break;
                case 'x':
                    if (size == SIZE_SHORT)
                        itos((uint16_t) va_arg(args, unsigned int), 16, tmp_buffer);
                    else if (size == SIZE_LONG)
                        itos((uint64_t) va_arg(args, unsigned long), 16, tmp_buffer);
                    else
                        itos((uint32_t) va_arg(args, unsigned int), 16, tmp_buffer);

                    snprintf_common(padchar, padlen, tmp_buffer);
                    break;
                case 'p':
#if BIT64
                    itos((uint64_t) va_arg(args, unsigned long), 16, tmp_buffer);
#elif BIT32
                    itos((uint32_t) va_arg(args, unsigned int), 16, tmp_buffer);
#else
#error "Environment is not 32 bit or 64 bit"
#endif

                    snprintf_common(padchar, padlen, tmp_buffer);
                    break;
                default:
                    if (len > 0) {
                        *buffer = c;
                        buffer++;

                        len--;
                    }

                    break;
                }
                continue;
            }

            if (len > 0) {
                *buffer = c;
                buffer++;

                len--;
            }
        } while (*++format != '\0');

        *buffer = '\0';
    }
}