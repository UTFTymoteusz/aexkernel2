#pragma once

#include "aex/dev/input.hpp"
#include "aex/mem.hpp"
#include "aex/spinlock.hpp"

#include <stddef.h>
#include <stdint.h>

extern "C" struct multiboot_info;
typedef struct multiboot_info multiboot_info_t;

namespace AEX::Dev::Input {
    void init();
    void tty_input_thread();
}

namespace AEX::TTY {
    constexpr auto ROOT_TTY   = 0;
    constexpr auto TTY_AMOUNT = 1;

    enum ansi_color_t {
        ANSI_FG_BLACK        = 30,
        ANSI_FG_RED          = 31,
        ANSI_FG_GREEN        = 32,
        ANSI_FG_BROWN        = 33,
        ANSI_FG_BLUE         = 34,
        ANSI_FG_PURPLE       = 35,
        ANSI_FG_CYAN         = 36,
        ANSI_FG_GRAY         = 37,
        ANSI_BG_BLACK        = 40,
        ANSI_BG_RED          = 41,
        ANSI_BG_GREEN        = 42,
        ANSI_BG_BROWN        = 43,
        ANSI_BG_BLUE         = 44,
        ANSI_BG_PURPLE       = 45,
        ANSI_BG_CYAN         = 46,
        ANSI_BG_GRAY         = 47,
        ANSI_FG_DARK_GRAY    = 90,
        ANSI_FG_LIGHT_RED    = 91,
        ANSI_FG_LIGHT_GREEN  = 92,
        ANSI_FG_YELLOW       = 93,
        ANSI_FG_LIGHT_BLUE   = 94,
        ANSI_FG_LIGHT_PURPLE = 95,
        ANSI_FG_LIGHT_CYAN   = 96,
        ANSI_FG_WHITE        = 97,
        ANSI_BG_DARK_GRAY    = 100,
        ANSI_BG_LIGHT_RED    = 101,
        ANSI_BG_LIGHT_GREEN  = 102,
        ANSI_BG_YELLOW       = 103,
        ANSI_BG_LIGHT_BLUE   = 104,
        ANSI_BG_LIGHT_PURPLE = 105,
        ANSI_BG_LIGHT_CYAN   = 106,
        ANSI_BG_WHITE        = 107,
    };

    /**
     * A basic virtual terminal class.
     */
    class VTTY {
        public:
        int width, height;

        VTTY();
        VTTY(int width, int height);

        /**
         * Reads a character from the virtual terminal.
         * @returns A character.
         */
        char readChar();

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

        /**
         * Sets the keymap of the virtual terminal.
         * @param _keymap Pointer to the new keymap. Will be copied over.
         */
        void set_keymap(Dev::Input::keymap* _keymap);

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

        VTTY& operator<<(ansi_color_t color);

        private:
        int _bgColor;
        int _fgColor;

        Mem::CircularBuffer* _inputBuffer;
        Dev::Input::keymap   _keymap = Dev::Input::default_keymap;

        void inputReady();
        void inputKeyPress(Dev::Input::event _event);

        friend void AEX::Dev::Input::init();
        friend void AEX::Dev::Input::tty_input_thread();

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
}