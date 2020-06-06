#pragma once

#include "aex/tty.hpp"

#include "boot/mboot.h"

#include <stdint.h>

namespace AEX::TTY {
    class GrTTY : public VTTY {
        public:
        struct vga_char;

        GrTTY();
        GrTTY(multiboot_info_t* mbinfo);

        GrTTY& setColorANSI(int ansi);

        void scrollDown(int amnt);

        void fillFromEGA(vga_char* ega_buffer);

        private:
        enum color {
            BLACK        = 0x050808,
            BLUE         = 0x0000AA,
            GREEN        = 0x00AA00,
            CYAN         = 0x00AAAA,
            RED          = 0xAA0000,
            PURPLE       = 0x7F00AA,
            BROWN        = 0x7F4000,
            GRAY         = 0xAAAAAA,
            DARK_GRAY    = 0x555555,
            LIGHT_BLUE   = 0x2020E0,
            LIGHT_GREEN  = 0x20E020,
            LIGHT_CYAN   = 0x20E0E0,
            LIGHT_RED    = 0xE02020,
            LIGHT_PURPLE = 0x7F00E0,
            YELLOW       = 0xE0E020,
            WHITE        = 0xFFFFFF,
        };

        const uint32_t _ega_to_color[16]{
            BLACK,     BLUE,       GREEN,       CYAN,       RED,       PURPLE,       BROWN,  GRAY,
            DARK_GRAY, LIGHT_BLUE, LIGHT_GREEN, LIGHT_CYAN, LIGHT_RED, LIGHT_PURPLE, YELLOW, WHITE,
        };

        uint32_t _bgColor = color::BLACK;
        uint32_t _fgColor = color::WHITE;

        uint32_t _px_width, _px_height;

        static uint32_t* volatile _output;
        uint32_t* _double_buffer;

        void put_glyph(char c, uint32_t x, uint32_t y, uint32_t fg, uint32_t bg);

        protected:
        void _writeChar(char c);
    };
}
