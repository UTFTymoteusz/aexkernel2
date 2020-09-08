#pragma once

#include "aex/dev/input.hpp"
#include "aex/dev/tty/ansi.hpp"
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

namespace AEX::Dev::TTY {
    constexpr auto ROOT_TTY   = 0;
    constexpr auto TTY_AMOUNT = 2;

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
        char read();

        /**
         * Reads a line into the specified buffer. Stops on \r or when the buffer is full (counting
         * the null byte at the end).
         */
        char* readLine(char* buffer, size_t len);

        /**
         * Writes a character to the virtual terminal.
         * @param str The character to write out.
         */
        VTTY& write(char c);

        /**
         * Writes a string to the virtual terminal.
         * @param str The string to write out.
         */
        VTTY& write(const char* str);

        /**
         * Sets the foreground or background color.
         * @param ansi An ANSI color code.
         */
        virtual VTTY& color(ansi_color_t ansi);

        /**
         * Scrolls down the virtual terminal.
         * @param amnt Amount of lines to scroll down by.
         */
        virtual VTTY& scroll(int amnt);

        /**
         * Sets the keymap of the virtual terminal.
         * @param m_keymap Pointer to the new keymap. Will be copied over.
         */
        void set_keymap(Dev::Input::keymap* m_keymap);

        int getCursorX() {
            return m_cursorx;
        }

        int getCursorY() {
            return m_cursory;
        }

        VTTY& setCursorX(int x) {
            m_cursorx = x;
            return *this;
        }

        VTTY& setCursorY(int y) {
            m_cursory = y;
            return *this;
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
        int m_bg;
        int m_fg;

        Mem::CircularBuffer* m_inputBuffer;
        Dev::Input::keymap   m_keymap = Dev::Input::default_keymap;

        void inputReady();
        void inputKeyPress(Dev::Input::event m_event);

        friend void AEX::Dev::Input::init();
        friend void AEX::Dev::Input::tty_input_thread();

        protected:
        int m_cursorx = 0;
        int m_cursory = 0;

        Spinlock m_lock;

        virtual void _write(char c);
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