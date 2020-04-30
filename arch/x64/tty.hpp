#pragma once

#include "aex/spinlock.hpp"

#include <stdint.h>

namespace AEX::TTY {
    constexpr auto ROOT_TTY   = 0;
    constexpr auto TTY_AMOUNT = 8;

    static constexpr auto TTY_WIDTH  = 80;
    static constexpr auto TTY_HEIGHT = 25;

    enum vga_color {
        VGA_BLACK        = 0,
        VGA_BLUE         = 1,
        VGA_GREEN        = 2,
        VGA_CYAN         = 3,
        VGA_RED          = 4,
        VGA_PURPLE       = 5,
        VGA_BROWN        = 6,
        VGA_GRAY         = 7,
        VGA_DARK_GRAY    = 8,
        VGA_LIGHT_BLUE   = 9,
        VGA_LIGHT_GREEN  = 10,
        VGA_LIGHT_CYAN   = 11,
        VGA_LIGHT_RED    = 12,
        VGA_LIGHT_PURPLE = 13,
        VGA_YELLOW       = 14,
        VGA_WHITE        = 15,
    };

    struct vga_char {
        char    ascii;
        uint8_t fg : 4;
        uint8_t bg : 4;
    } __attribute__((packed));

    /**
     * A basic virtual terminal class.
     */
    class VTTY {
      public:
        VTTY();
        VTTY(void* _outputB);

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
        void setColorANSI(int ansi);

        /**
         * Scrolls down the virtual terminal.
         * @param amnt Amount of lines to scroll down by.
         */
        void scrollDown(int amnt);

      private:
        int _cursorx = 0;
        int _cursory = 0;

        int _bgColor = VGA_BLACK;
        int _fgColor = VGA_WHITE;

        vga_char* volatile _output;

        Spinlock _lock;

        void _writeChar(char c);
    };

    /**
     * An array of all virtual terminals.
     */
    extern VTTY VTTYs[TTY_AMOUNT];

    void init();
} // namespace AEX::TTY