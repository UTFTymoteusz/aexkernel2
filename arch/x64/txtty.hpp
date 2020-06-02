#pragma once

#include "aex/tty.hpp"

namespace AEX::TTY {
    class TxTTY : public VTTY {
      public:
        struct vga_char;

        TxTTY();
        TxTTY(void* output);

        static void clear();

        TxTTY& setColorANSI(int ansi);

        void scrollDown(int amnt);

        vga_char* getOutputPointer();

      private:
        enum vga_color {
            VGA_BLACK        = 0,
            VGA_BLUE         = 1,
            VGA_GREEN        = 2,
            VGA_CYAN         = 3,
            VGA_RED          = 4,
            VGA_PURPLE       = 5,
            VGA_BROWN        = 6,
            VGA_GRAY         = 7,
            VGA_DARK_GRAY    = 8,
            VGA_LIGHT_BLUE   = 9,
            VGA_LIGHT_GREEN  = 10,
            VGA_LIGHT_CYAN   = 11,
            VGA_LIGHT_RED    = 12,
            VGA_LIGHT_PURPLE = 13,
            VGA_YELLOW       = 14,
            VGA_WHITE        = 15,
        };

        int _bgColor = VGA_BLACK;
        int _fgColor = VGA_WHITE;

        vga_char* volatile _output;

      protected:
        void _writeChar(char c);
    };
}