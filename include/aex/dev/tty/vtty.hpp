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
    /**
     * A basic virtual terminal class.
     **/
    class API VTTY {
        public:
        int width, height;

        VTTY();
        VTTY(int width, int height);

        /**
         * Reads a character from the virtual terminal.
         * @returns A character.
         **/
        int read();

        /**
         * Writes a character to the virtual terminal.
         * @param str The character to write out.
         **/
        VTTY& write(char c);

        /**
         * Writes a string to the virtual terminal.
         * @param str The string to write out.
         **/
        VTTY& write(const char* str);

        /**
         * Sets the foreground or background color.
         * @param ansi An ANSI color code.
         **/
        virtual VTTY& color(ansi_color_t ansi) = 0;

        /**
         * Scrolls down the virtual terminal.
         * @param amnt Amount of lines to scroll down by.
         **/
        virtual VTTY& scroll(int amnt) = 0;

        /**
         * Clears the virtual terminal with the current background clor;
         **/
        virtual VTTY& clear() = 0;

        /**
         * Sets the keymap of the virtual terminal.
         * @param m_keymap Pointer to the new keymap. Will be copied over.
         **/
        void set_keymap(Dev::Input::keymap* m_keymap);

        virtual bool text()     = 0;
        virtual bool graphics() = 0;

        virtual tty_info info()   = 0;
        virtual void*    output() = 0;

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

        Proc::pid_t tcgetpgrp() {
            SCOPE(m_mutex);
            return m_foreground_pgrp;
        }

        void tcsetpgrp(Proc::pid_t pgrp) {
            SCOPE(m_mutex);
            m_foreground_pgrp = pgrp;
        }

        private:
        Mutex m_mutex;

        int m_bg;
        int m_fg;

        Mem::CircularBuffer<int, true>* m_inputBuffer;
        Dev::Input::keymap              m_keymap = Dev::Input::default_keymap;

        Proc::pid_t m_foreground_pgrp = -1;

        void inputReady();
        void keyPress(Dev::Input::event m_event);

        friend void AEX::Dev::Input::init();
        friend void AEX::Dev::Input::tty_loop();

        protected:
        int m_cursorx = 0;
        int m_cursory = 0;

        Spinlock m_lock;

        virtual void _write(char c) = 0;
    };
}