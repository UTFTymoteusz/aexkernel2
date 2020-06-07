#pragma once

#include "aex/spinlock.hpp"

#include <stddef.h>
#include <stdint.h>
extern "C" struct multiboot_info;
typedef struct multiboot_info multiboot_info_t;

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
        int _bgColor;
        int _fgColor;

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

    /**
     * Initializes the bare neccesities required for a terminal.
     */
    void init(multiboot_info_t* mbinfo);

    /**
     * Initializes all terminals and makes them actually appear if in framebuffer mode.
     */
    void init_mem(multiboot_info_t* mbinfo);
} // namespace AEX::TTY