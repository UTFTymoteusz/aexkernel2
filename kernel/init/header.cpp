#include "aex/tty.hpp"

#define COLOR0 94
#define COLOR1 97

namespace AEX::Init {
    // clang-format off
    const char header[] = "\
  |##### |##### \\#  /#   |##### \n\
  |#   # |#      \\#/#        |# \n\
  |##### |#####   \\#     |##### \n\
  |#   # |#      /#\\#    |#     \n\
  |#   # |##### /#  \\#   |##### \n";
    // clang-format on

    void init_print_header() {
        auto rootTTY = TTY::VTTYs[TTY::ROOT_TTY];

        char color = COLOR0;
        rootTTY->setColorANSI(COLOR0);

        for (size_t i = 0; i < sizeof(header) - 1; i++) {
            char c = header[i];

            switch (c) {
            case '\\':
            case '|':
            case '/':
                if (color != COLOR0) {
                    rootTTY->setColorANSI(COLOR0);
                    color = COLOR0;
                }
                break;
            default:
                if (color != COLOR1) {
                    rootTTY->setColorANSI(COLOR1);
                    color = COLOR1;
                }
                break;
            }
            rootTTY->writeChar(c);
        }
    }
}