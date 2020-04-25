#include "tty.hpp"

#include "kernel/string.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::TTY {
    VTTY::VTTY() { _output = nullptr; }

    VTTY::VTTY(void* _outputB) { _output = (vga_char_t*) _outputB; }

    void VTTY::_writeChar(char c) {
        switch (c) {
        case '\n':
            _cursorx = 0;
            _cursory++;
            break;
        case '\r':
            _cursorx = 0;
            break;
        default:
            _output[_cursorx + _cursory * TTY_WIDTH].ascii = c;
            _output[_cursorx + _cursory * TTY_WIDTH].bg    = _bgColor;
            _output[_cursorx + _cursory * TTY_WIDTH].fg    = _fgColor;

            _cursorx++;

            if (_cursorx >= TTY_WIDTH) {
                _cursorx = 0;
                _cursory++;
            }
            break;
        }

        if (_cursory >= TTY_HEIGHT) {
            _cursory--;
            scrollDown(1);
        }
    }

    void VTTY::writeChar(char c) {
        _lock.acquire();
        _writeChar(c);
        _lock.release();
    }

    void VTTY::write(const char* str) {
        _lock.acquire();

        while (*str != '\0')
            _writeChar(*str++);

        _lock.release();
    }

    void VTTY::setColorANSI(int ansi) {
        const char ansi_to_vga[16] = {
            VGA_BLACK,      VGA_RED,          VGA_GREEN,       VGA_BROWN,
            VGA_BLUE,       VGA_PURPLE,       VGA_CYAN,        VGA_GRAY,
            VGA_DARK_GRAY,  VGA_LIGHT_RED,    VGA_LIGHT_GREEN, VGA_YELLOW,
            VGA_LIGHT_BLUE, VGA_LIGHT_PURPLE, VGA_LIGHT_CYAN,  VGA_WHITE,
        };

        if (ansi >= 30 && ansi <= 37)
            _fgColor = ansi_to_vga[ansi - 30];
        else if (ansi >= 40 && ansi <= 47)
            _bgColor = ansi_to_vga[ansi - 40];
        else if (ansi >= 90 && ansi <= 97)
            _fgColor = ansi_to_vga[ansi - 90 + 8];
        else if (ansi >= 100 && ansi <= 107)
            _bgColor = ansi_to_vga[ansi - 100 + 8];
    }

    void VTTY::scrollDown(int amnt) {
        for (size_t i = 0; i < amnt; i++)
            memcpy(_output, &_output[TTY_WIDTH], TTY_WIDTH * (TTY_HEIGHT - 1) * 2);

        for (size_t i = TTY_WIDTH * (TTY_HEIGHT - 1); i < TTY_WIDTH * TTY_HEIGHT; i++) {
            _output[i].ascii = ' ';
            _output[i].bg    = _bgColor;
            _output[i].fg    = _fgColor;
        }
    }

    VTTY VTTYs[TTY_AMOUNT];

    static void clear() {
        vga_char* volatile buffer = (vga_char_t*) 0xFFFFFFFF800B8000;

        for (int y = 0; y < TTY_HEIGHT; y++)
            for (int x = 0; x < TTY_WIDTH; x++) {
                buffer[x + y * TTY_WIDTH].ascii = '\0';
                buffer[x + y * TTY_WIDTH].bg    = VGA_BLACK;
                buffer[x + y * TTY_WIDTH].fg    = VGA_WHITE;
            }
    }

    void init() {
        clear();

        for (int i = 0; i < TTY_AMOUNT; i++)
            VTTYs[i] = VTTY((void*) 0xFFFFFFFF800B8000);
    }
} // namespace AEX::TTY