#include "aex/printk.hpp"

#include "aex/string.hpp"

#include "tty.hpp"

#include <stdarg.h>
#include <stdint.h>

namespace AEX {
    enum size {
        SIZE_SHORT = 0,
        SIZE_INT   = 1,
        SIZE_LONG  = 2,
    };

    Spinlock lock;

    void printk(const char* format, ...) {
        va_list args;
        va_start(args, format);

        printk(format, args);

        va_end(args);
    }

    void printk(const char* format, va_list args) {
        auto scopeLock = ScopeSpinlock(lock);

        auto rootTTY = TTY::VTTYs[TTY::ROOT_TTY];

        auto printk_common = [rootTTY](char padchar, int padlen, char* buffer) {
            for (int i = strlen(buffer); i < padlen; i++)
                *rootTTY << padchar;

            *rootTTY << buffer;
        };

        static bool newline = true;

        if (newline) {
            newline = false;

            if (format[0] == '^') {
                switch (format[1]) {
                case '0':
                    rootTTY->setColorANSI(90);
                    rootTTY->write(" [ ");

                    rootTTY->setColorANSI(94);
                    rootTTY->write("INIT");

                    rootTTY->setColorANSI(90);
                    rootTTY->write(" ] ");

                    rootTTY->setColorANSI(97);
                    break;
                case '1':
                    rootTTY->setColorANSI(90);
                    rootTTY->write(" [  ");

                    rootTTY->setColorANSI(92);
                    rootTTY->write("OK");

                    rootTTY->setColorANSI(90);
                    rootTTY->write("  ] ");

                    rootTTY->setColorANSI(97);
                    break;
                case '2':
                    rootTTY->setColorANSI(90);
                    rootTTY->write(" [ ");

                    rootTTY->setColorANSI(93);
                    rootTTY->write("WARN");

                    rootTTY->setColorANSI(90);
                    rootTTY->write(" ] ");

                    rootTTY->setColorANSI(97);
                    break;
                case '3':
                    rootTTY->setColorANSI(90);
                    rootTTY->write(" [ ");

                    rootTTY->setColorANSI(91);
                    rootTTY->write("FAIL");

                    rootTTY->setColorANSI(90);
                    rootTTY->write(" ] ");

                    rootTTY->setColorANSI(97);
                    break;
                default:
                    break;
                }
                format += 2;
            }
            else
                rootTTY->write("  ");
        }

        char buffer[72];

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
                    buffer[ptr++] = c;

                    c = *++format;
                }
                buffer[ptr] = '\0';

                if (ptr != 0)
                    padlen = stoi<int>(10, buffer);

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
                case '$':
                    if (padlen == 0)
                        padlen = 97;

                    rootTTY->setColorANSI(padlen);
                    break;
                case 's':
                    printk_common(padchar, padlen, va_arg(args, char*));
                    break;
                case 'c':
                    buffer[0] = (char) va_arg(args, int);
                    buffer[1] = '\0';

                    printk_common(padchar, padlen, buffer);
                    break;
                case 'i':
                    if (size == SIZE_SHORT)
                        itos((int16_t) va_arg(args, int), 10, buffer);
                    else if (size == SIZE_LONG)
                        itos((int64_t) va_arg(args, long), 10, buffer);
                    else
                        itos((int32_t) va_arg(args, int), 10, buffer);

                    printk_common(padchar, padlen, buffer);
                    break;
                case 'u':
                    if (size == SIZE_SHORT)
                        itos((uint16_t) va_arg(args, unsigned int), 10, buffer);
                    else if (size == SIZE_LONG)
                        itos((uint64_t) va_arg(args, unsigned long), 10, buffer);
                    else
                        itos((uint32_t) va_arg(args, unsigned int), 10, buffer);

                    printk_common(padchar, padlen, buffer);
                    break;
                case 'x':
                    if (size == SIZE_SHORT)
                        itos((uint16_t) va_arg(args, unsigned int), 16, buffer);
                    else if (size == SIZE_LONG)
                        itos((uint64_t) va_arg(args, unsigned long), 16, buffer);
                    else
                        itos((uint32_t) va_arg(args, unsigned int), 16, buffer);

                    printk_common(padchar, padlen, buffer);
                    break;
                case 'p':
#if INTPTR_MAX == INT64_MAX
                    itos((uint64_t) va_arg(args, unsigned long), 16, buffer);
#elif INTPTR_MAX == INT32_MAX
                    itos((uint32_t) va_arg(args, unsigned int), 16, buffer);
#else
#error "Environment is not 32 bit or 64 bit"
#endif

                    printk_common(padchar, padlen, buffer);
                    break;
                default:
                    rootTTY->writeChar(c);
                    break;
                }
                continue;
            }

            if (c == '\n')
                newline = true;

            TTY::VTTYs[0]->writeChar(c);
        } while (*++format != '\0');
    }
} // namespace AEX