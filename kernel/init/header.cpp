#include "aex/dev/tty.hpp"
#include "aex/printk.hpp"

namespace AEX::Init {
    constexpr auto COLOR0 = Dev::TTY::ANSI_FG_BLUE;
    constexpr auto COLOR1 = Dev::TTY::ANSI_FG_WHITE;

    // clang-format off
    const char header[] = "\
  |##### |##### \\#  /#   |##### \n\
  |#   # |#      \\#/#        |# \n\
  |##### |#####   \\#     |##### \n\
  |#   # |#      /#\\#    |#     \n\
  |#   # |##### /#  \\#   |##### \n";
    // clang-format on

    void init_print_header() {
        auto rootTTY = Dev::TTY::VTTYs[Dev::TTY::ROOT_TTY];

        char color = COLOR0;
        rootTTY->color(COLOR0);

        for (size_t i = 0; i < sizeof(header) - 1; i++) {
            char c = header[i];

            switch (c) {
            case '\\':
            case '|':
            case '/':
                if (color != COLOR0) {
                    rootTTY->color(COLOR0);
                    color = COLOR0;
                }
                break;
            default:
                if (color != COLOR1) {
                    rootTTY->color(COLOR1);
                    color = COLOR1;
                }
                break;
            }

            rootTTY->write(c);
        }
    }
}