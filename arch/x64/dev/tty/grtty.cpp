#include "grtty.hpp"

#include "aex/mem.hpp"
#include "aex/string.hpp"

#include "boot/font.hpp"

constexpr auto GLYPH_WIDTH = 8;

namespace AEX::Dev::TTY {
    uint32_t* volatile GrTTY::m_output = nullptr;

    struct GrTTY::vga_char {
        char    ascii;
        uint8_t fg : 4;
        uint8_t bg : 4;
    };

    GrTTY::GrTTY() {}

    GrTTY::GrTTY(multiboot_info_t* mbinfo) {
        if (mbinfo->framebuffer_bpp != 32)
            asm volatile("ud2");

        m_px_width  = mbinfo->framebuffer_width;
        m_px_height = mbinfo->framebuffer_height;

        uint32_t total_len = m_px_width * m_px_height * sizeof(uint32_t);

        if (!m_output) {
            m_output = (uint32_t*) Mem::kernel_pagemap->map(total_len, mbinfo->framebuffer_addr,
                                                            PAGE_COMBINE | PAGE_WRITE);
            memset32(m_output, m_bg, m_px_width * m_px_height);
        }

        m_double_buffer = (uint32_t*) Mem::kernel_pagemap->alloc(total_len);
        memset32(m_double_buffer, m_bg, m_px_width * m_px_height);

        width  = m_px_width / GLYPH_WIDTH;
        height = m_px_height / psf_font->size;
    }

    void GrTTY::fromVGA(vga_char* ega_buffer) {
        for (int y = 0; y < 25; y++)
            for (int x = 0; x < 80; x++) {
                auto c = ega_buffer[x + y * 80];
                put(c.ascii, x, y, m_ega_to_color[c.fg], m_ega_to_color[c.bg]);
            }

        m_cursory = 25;
    }

    void GrTTY::put(char c, uint32_t x, uint32_t y, rgb_t fg, rgb_t bg) {
        int height = psf_font->size;

        x *= GLYPH_WIDTH;
        y *= height;

        uint8_t* current = psf_font->data + c * height;

        for (int cy = 0; cy < height; cy++) {
            rgb_t* dst = m_output + x + m_px_width * (y + cy);

            for (int cx = GLYPH_WIDTH; cx > 0; cx--) {
                *dst = (*current & (1 << cx)) ? fg : bg;
                dst++;
            }

            dst = m_double_buffer + x + m_px_width * (y + cy);

            for (int cx = GLYPH_WIDTH; cx > 0; cx--) {
                *dst = (*current & (1 << cx)) ? fg : bg;
                dst++;
            }

            current++;
        }
    }

    GrTTY& GrTTY::color(ansi_color_t ansi) {
        const rgb_t ansi_to_color[16] = {
            RGB_BLACK,      RGB_RED,          RGB_GREEN,       RGB_BROWN,
            RGB_BLUE,       RGB_PURPLE,       RGB_CYAN,        RGB_GRAY,
            RGB_DARK_GRAY,  RGB_LIGHT_RED,    RGB_LIGHT_GREEN, RGB_YELLOW,
            RGB_LIGHT_BLUE, RGB_LIGHT_PURPLE, RGB_LIGHT_CYAN,  RGB_WHITE,
        };

        if (ansi >= 30 && ansi <= 37)
            m_fg = ansi_to_color[ansi - 30];
        else if (ansi >= 40 && ansi <= 47)
            m_bg = ansi_to_color[ansi - 40];
        else if (ansi >= 90 && ansi <= 97)
            m_fg = ansi_to_color[ansi - 90 + 8];
        else if (ansi >= 100 && ansi <= 107)
            m_bg = ansi_to_color[ansi - 100 + 8];

        return *this;
    }

    GrTTY& GrTTY::scroll(int amnt) {
        memcpy(m_double_buffer, &m_double_buffer[m_px_width * psf_font->size * amnt],
               m_px_width * (m_px_height - psf_font->size * amnt) * sizeof(uint32_t));

        memset32(&m_double_buffer[m_px_width * (m_px_height - psf_font->size)], m_bg,
                 m_px_width * psf_font->size);

        memcpy(m_output, m_double_buffer, m_px_width * m_px_height * sizeof(uint32_t));

        return *this;
    }

    void GrTTY::_write(char c) {
        switch (c) {
        case '\n':
            m_cursorx = 0;
            m_cursory++;
            break;
        case '\r':
            m_cursorx = 0;
            break;
        default:
            put(c, m_cursorx, m_cursory, m_fg, m_bg);

            m_cursorx++;

            if (m_cursorx >= width) {
                m_cursorx = 0;
                m_cursory++;
            }

            break;
        }

        if (m_cursory >= height) {
            m_cursory--;
            scroll(1);
        }
    }
}