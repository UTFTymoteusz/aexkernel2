#pragma once

#include "aex/spinlock.hpp"

#include "boot/mboot.h"

#include <stdint.h>

namespace AEX::TTY {
    constexpr auto ROOT_TTY   = 0;
    constexpr auto TTY_AMOUNT = 1;

    /**
     * A basic virtual terminal class.
     */
    class VTTY {
      public:
        int width, height;

        VTTY();
        VTTY(int width, int height);

        /**
         * Writes a character to the virtual terminal.
         * @param str The character to write out.
         */
        void writeChar(char c);

        /**
         * Writes a string to the virtual terminal.
         * @param str The string to write out.
         */
        void write(const char* str);

        /**
         * Sets the foreground or background color.
         * @param ansi An ANSI color code.
         */
        virtual VTTY& setColorANSI(int ansi);

        /**
         * Scrolls down the virtual terminal.
         * @param amnt Amount of lines to scroll down by.
         */
        virtual void scrollDown(int amnt);

        int getCursorX() {
            return _cursorx;
        }

        int getCursorY() {
            return _cursory;
        }

        void setCursorX(int x) {
            _cursorx = x;
        }

        void setCursorY(int y) {
            _cursory = y;
        }

        VTTY& operator<<(bool val);

        VTTY& operator<<(char c);

        VTTY& operator<<(int8_t val);
        VTTY& operator<<(uint8_t val);

        VTTY& operator<<(int16_t val);
        VTTY& operator<<(uint16_t val);

        VTTY& operator<<(int32_t val);
        VTTY& operator<<(uint32_t val);

        VTTY& operator<<(int64_t val);
        VTTY& operator<<(uint64_t val);

        VTTY& operator<<(void* ptr);

        VTTY& operator<<(const char* str);

      private:
        enum color {
            COLOR_BLACK        = 0,
            COLOR_BLUE         = 1,
            COLOR_GREEN        = 2,
            COLOR_CYAN         = 3,
            COLOR_RED          = 4,
            COLOR_PURPLE       = 5,
            COLOR_BROWN        = 6,
            COLOR_GRAY         = 7,
            COLOR_DARK_GRAY    = 8,
            COLOR_LIGHT_BLUE   = 9,
            COLOR_LIGHT_GREEN  = 10,
            COLOR_LIGHT_CYAN   = 11,
            COLOR_LIGHT_RED    = 12,
            COLOR_LIGHT_PURPLE = 13,
            COLOR_YELLOW       = 14,
            COLOR_WHITE        = 15,
        };

        int _bgColor = COLOR_BLACK;
        int _fgColor = COLOR_WHITE;

      protected:
        int _cursorx = 0;
        int _cursory = 0;

        Spinlock _lock;

        virtual void _writeChar(char c);
    };

    /**
     * An array of all virtual terminals.
     */
    extern VTTY* VTTYs[TTY_AMOUNT];

    void init(multiboot_info_t* mbinfo);

    void init_mem(multiboot_info_t* mbinfo);
} // namespace AEX::TTY