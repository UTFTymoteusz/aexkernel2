#pragma once

#include "aex/dev/input.hpp"
#include "aex/dev/tty.hpp"
#include "aex/dev/tty/ansi.hpp"
#include "aex/utility.hpp"

#include <stddef.h>
#include <stdint.h>

extern "C" struct multiboot_info;
typedef struct multiboot_info multiboot_info_t;

namespace AEX::Dev::TTY {
    struct API tty_info {
        int width;
        int height;

        int gr_width;
        int gr_height;
        int gr_depth;
        int gr_bytes;
    };

    /**
     * A basic terminal class.
     **/
    class API TTY {
        public:
        /**
         * Reads a character from the virtual terminal.
         * @returns A character.
         **/
        virtual int read();

        /**
         * Reads a line into the specified buffer. Stops on \r or when the buffer is full (counting
         * the null byte at the end).
         **/
        char* readLine(char* buffer, size_t len);

        /**
         * Writes a character to the terminal.
         * @param str The character to write out.
         **/
        virtual TTY& write(char c);

        /**
         * Writes a string to the terminal.
         * @param str The string to write out.
         **/
        virtual TTY& write(const char* str);

        virtual bool text();
        virtual bool graphics();

        virtual tty_info info();
        virtual void*    output();

        /**
         * Sets the foreground or background color.
         * @param ansi An ANSI color code.
         **/
        TTY& color(ansi_color_t ansi);

        TTY& operator<<(bool val);

        TTY& operator<<(char c);

        TTY& operator<<(int8_t val);
        TTY& operator<<(uint8_t val);

        TTY& operator<<(int16_t val);
        TTY& operator<<(uint16_t val);

        TTY& operator<<(int32_t val);
        TTY& operator<<(uint32_t val);

        TTY& operator<<(int64_t val);
        TTY& operator<<(uint64_t val);

        TTY& operator<<(void* ptr);

        TTY& operator<<(const char* str);

        TTY& operator<<(ansi_color_t color);
    };
}