#include "grtty.hpp"

#include "aex/mem.hpp"
#include "aex/string.hpp"

#include "boot/font.hpp"

constexpr auto GLYPH_WIDTH = 8;

namespace AEX::TTY {
    uint32_t* volatile GrTTY::_output = nullptr;

    struct GrTTY::vga_char {
        char    ascii;
        uint8_t fg : 4;
        uint8_t bg : 4;
    };

    GrTTY::GrTTY() {}

    GrTTY::GrTTY(multiboot_info_t* mbinfo) {
        if (mbinfo->framebuffer_bpp != 32)
            asm volatile("ud2");

        _px_width  = mbinfo->framebuffer_width;
        _px_height = mbinfo->framebuffer_height;

        uint32_t total_len = _px_width * _px_height * sizeof(uint32_t);

        if (!_output) {
            _output = (uint32_t*) Mem::kernel_pagemap->map(total_len, mbinfo->framebuffer_addr,
                                                           PAGE_COMBINE | PAGE_WRITE);
            memset32(_output, _bgColor, _px_width * _px_height);
        }

        _double_buffer = (uint32_t*) Mem::kernel_pagemap->alloc(total_len);
        memset32(_double_buffer, _bgColor, _px_width * _px_height);

        width  = _px_width / GLYPH_WIDTH;
        height = _px_height / psf_font->size;
    }

    void GrTTY::fillFromEGA(vga_char* ega_buffer) {
        for (int y = 0; y < 25; y++)
            for (int x = 0; x < 80; x++) {
                auto c = ega_buffer[x + y * 80];
                put_glyph(c.ascii, x, y, _ega_to_color[c.fg], _ega_to_color[c.bg]);
            }

        _cursory = 25;
    }

    void GrTTY::put_glyph(char c, uint32_t x, uint32_t y, uint32_t fg, uint32_t bg) {
        int height = psf_font->size;

        x *= GLYPH_WIDTH;
        y *= height;

        uint8_t* current = psf_font->data + c * height;

        for (int cy = 0; cy < height; cy++) {
            uint32_t* dst = _output + x + _px_width * (y + cy);

            for (int cx = GLYPH_WIDTH; cx > 0; cx--) {
                *dst = (*current & (1 << cx)) ? fg : bg;
                dst++;
            }

            dst = _double_buffer + x + _px_width * (y + cy);

            for (int cx = GLYPH_WIDTH; cx > 0; cx--) {
                *dst = (*current & (1 << cx)) ? fg : bg;
                dst++;
            }

            current++;
        }
    }

    GrTTY& GrTTY::setColorANSI(int ansi) {
        const uint32_t ansi_to_color[16] = {
            BLACK,     RED,       GREEN,       BROWN,  BLUE,       PURPLE,       CYAN,       GRAY,
            DARK_GRAY, LIGHT_RED, LIGHT_GREEN, YELLOW, LIGHT_BLUE, LIGHT_PURPLE, LIGHT_CYAN, WHITE,
        };

        if (ansi >= 30 && ansi <= 37)
            _fgColor = ansi_to_color[ansi - 30];
        else if (ansi >= 40 && ansi <= 47)
            _bgColor = ansi_to_color[ansi - 40];
        else if (ansi >= 90 && ansi <= 97)
            _fgColor = ansi_to_color[ansi - 90 + 8];
        else if (ansi >= 100 && ansi <= 107)
            _bgColor = ansi_to_color[ansi - 100 + 8];

        return *this;
    }

    void GrTTY::scrollDown(int amnt) {
        memcpy(_double_buffer, &_double_buffer[_px_width * psf_font->size * amnt],
               _px_width * (_px_height - psf_font->size * amnt) * sizeof(uint32_t));

        memset32(&_double_buffer[_px_width * (_px_height - psf_font->size)], _bgColor,
                 _px_width * psf_font->size);

        memcpy(_output, _double_buffer, _px_width * _px_height * sizeof(uint32_t));
    }

    void GrTTY::_writeChar(char c) {
        switch (c) {
        case '\n':
            _cursorx = 0;
            _cursory++;
            break;
        case '\r':
            _cursorx = 0;
            break;
        default:
            put_glyph(c, _cursorx, _cursory, _fgColor, _bgColor);

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