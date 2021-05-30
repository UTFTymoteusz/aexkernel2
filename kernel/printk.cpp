#include "aex/printk.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/dev/tty.hpp"
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

    Spinlock printk_lock;
    int      faulted_cpu = -1;

    void printk(const char* format, ...) {
        va_list args;
        va_start(args, format);

        printk(format, args);

        va_end(args);
    }

    void printk(const char* format, va_list args) {
        using namespace AEX::Dev::TTY;

        if (faulted_cpu == -1)
            printk_lock.acquire();
        else if (Sys::CPU::currentID() != faulted_cpu)
            return; // printk_lock.acquire();

        auto rootTTY = TTYs[ROOT_TTY];

        auto printk_common = [rootTTY](char padchar, size_t padlen, char* buffer) {
            for (size_t i = strlen(buffer); i < padlen; i++)
                *rootTTY << padchar;

            *rootTTY << buffer;
        };

        static bool newline = true;

        if (newline) {
            newline = false;
            *rootTTY << "  ";
        }

        char buffer[72];

        do {
            char c = *format;

            if (c == '%') {
                size_t padlen  = 0;
                char   padchar = ' ';

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

                    rootTTY->color((ansi_color_t) padlen);
                    break;
                case '^':
                    *rootTTY << "\b\b";

                    switch (padlen) {
                    case 0:
                        *rootTTY << ANSI_FG_DARK_GRAY << " [ " << ANSI_FG_LIGHT_BLUE << "INIT"
                                 << ANSI_FG_DARK_GRAY << " ] " << ANSI_FG_WHITE;
                        break;
                    case 1:
                        *rootTTY << ANSI_FG_DARK_GRAY << " [  " << ANSI_FG_LIGHT_GREEN << "OK"
                                 << ANSI_FG_DARK_GRAY << "  ] " << ANSI_FG_WHITE;
                        break;
                    case 2:
                        *rootTTY << ANSI_FG_DARK_GRAY << " [ " << ANSI_FG_YELLOW << "WARN"
                                 << ANSI_FG_DARK_GRAY << " ] " << ANSI_FG_WHITE;
                        break;
                    case 3:
                        *rootTTY << ANSI_FG_DARK_GRAY << " [ " << ANSI_FG_LIGHT_RED << "FAIL"
                                 << ANSI_FG_DARK_GRAY << " ] " << ANSI_FG_WHITE;
                        break;
                    default:
                        break;
                    }
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
                case 'X':
                    if (size == SIZE_SHORT)
                        itos((uint16_t) va_arg(args, unsigned int), 16, buffer);
                    else if (size == SIZE_LONG)
                        itos((uint64_t) va_arg(args, unsigned long), 16, buffer);
                    else
                        itos((uint32_t) va_arg(args, unsigned int), 16, buffer);

                    for (size_t i = 0; i < sizeof(buffer); i++)
                        buffer[i] = toupper(buffer[i]);

                    printk_common(padchar, padlen, buffer);
                    break;
                case 'p':
#if BIT64
                    itos((uint64_t) va_arg(args, unsigned long), 16, buffer);
#elif BIT32
                    itos((uint32_t) va_arg(args, unsigned int), 16, buffer);
#else
#error "Environment is not 32 bit or 64 bit"
#endif

                    printk_common(padchar, padlen, buffer);
                    break;
                default:
                    *rootTTY << c;
                    break;
                }
                continue;
            }

            if (c == '\n' || c == '\r')
                newline = true;

            *rootTTY << c;
        } while (*++format != '\0');

        if (faulted_cpu == -1)
            printk_lock.release();
    }

    void printk_fault() {
        interruptible(false) {
            faulted_cpu = Sys::CPU::currentID();
        }
    }

    void printk_nofault() {
        interruptible(false) {
            faulted_cpu = -1;
        }
    }
}