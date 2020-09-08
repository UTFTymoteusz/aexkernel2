#pragma once

#include "aex/dev/tty.hpp"

#include "boot/mboot.h"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Dev::TTY {
    class GrTTY : public VTTY {
        public:
        struct vga_char;

        GrTTY();
        GrTTY(multiboot_info_t* mbinfo);

        GrTTY& color(ansi_color_t ansi);

        GrTTY& scroll(int amnt);

        void fromVGA(vga_char* ega_buffer);

        private:
        enum color {
            RGB_BLACK        = 0x050808,
            RGB_BLUE         = 0x0000AA,
            RGB_GREEN        = 0x00AA00,
            RGB_CYAN         = 0x00AAAA,
            RGB_RED          = 0xAA0000,
            RGB_PURPLE       = 0x7F00AA,
            RGB_BROWN        = 0x7F4000,
            RGB_GRAY         = 0xAAAAAA,
            RGB_DARK_GRAY    = 0x555555,
            RGB_LIGHT_BLUE   = 0x2020E0,
            RGB_LIGHT_GREEN  = 0x20E020,
            RGB_LIGHT_CYAN   = 0x20E0E0,
            RGB_LIGHT_RED    = 0xE02020,
            RGB_LIGHT_PURPLE = 0x7F00E0,
            RGB_YELLOW       = 0xE0E020,
            RGB_WHITE        = 0xFFFFFF,
        };

        const uint32_t m_ega_to_color[16]{
            RGB_BLACK,     RGB_BLUE,         RGB_GREEN,       RGB_CYAN,
            RGB_RED,       RGB_PURPLE,       RGB_BROWN,       RGB_GRAY,
            RGB_DARK_GRAY, RGB_LIGHT_BLUE,   RGB_LIGHT_GREEN, RGB_LIGHT_CYAN,
            RGB_LIGHT_RED, RGB_LIGHT_PURPLE, RGB_YELLOW,      RGB_WHITE,
        };

        uint32_t m_bg = RGB_BLACK;
        uint32_t m_fg = RGB_WHITE;

        uint32_t m_px_width, m_px_height;

        static uint32_t* volatile m_output;
        uint32_t* m_double_buffer;

        void put_glyph(char c, uint32_t x, uint32_t y, uint32_t fg, uint32_t bg);

        protected:
        void _write(char c);
    };
}
