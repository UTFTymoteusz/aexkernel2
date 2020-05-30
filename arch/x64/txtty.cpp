#include "txtty.hpp"

#include "aex/string.hpp"

#include "sys/cpu.hpp"

namespace AEX::TTY {
    struct TxTTY::vga_char {
        char    ascii;
        uint8_t fg : 4;
        uint8_t bg : 4;
    };

    TxTTY::TxTTY() {}

    TxTTY::TxTTY(void* output) {
        width  = 80;
        height = 25;

        _output = (vga_char*) output;
    }

    TxTTY::vga_char* TxTTY::getOutputPointer() {
        return _output;
    }

    void TxTTY::clear() {
        vga_char* volatile buffer = (vga_char*) 0xFFFFFFFF800B8000;

        const int width  = 80;
        const int height = 25;

        for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++) {
                buffer[x + y * width].ascii = '\0';
                buffer[x + y * width].bg    = VGA_BLACK;
                buffer[x + y * width].fg    = VGA_BLUE;
            }
    }

    TxTTY& TxTTY::setColorANSI(int ansi) {
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

        return *this;
    }

    void TxTTY::scrollDown(int amnt) {
        for (int i = 0; i < amnt; i++)
            memcpy(_output, &_output[width], width * (height - 1) * 2);

        for (int i = width * (height - 1); i < width * height; i++) {
            _output[i].ascii = ' ';
            _output[i].bg    = _bgColor;
            _output[i].fg    = _fgColor;
        }
    }

    void TxTTY::_writeChar(char c) {
        switch (c) {
        case '\n':
            _cursorx = 0;
            _cursory++;
            break;
        case '\r':
            _cursorx = 0;
            break;
        default:
            _output[_cursorx + _cursory * width].ascii = c;
            _output[_cursorx + _cursory * width].bg    = _bgColor;
            _output[_cursorx + _cursory * width].fg    = _fgColor;

            _cursorx++;

            if (_cursorx >= width) {
                _cursorx = 0;
                _cursory++;
            }

            break;
        }

        if (_cursory >= height) {
            _cursory--;
            scrollDown(1);
        }
    }
}