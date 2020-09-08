#pragma once

namespace AEX::Dev::TTY {
    enum ansi_color_t {
        ANSI_FG_BLACK        = 30,
        ANSI_FG_RED          = 31,
        ANSI_FG_GREEN        = 32,
        ANSI_FG_BROWN        = 33,
        ANSI_FG_BLUE         = 34,
        ANSI_FG_PURPLE       = 35,
        ANSI_FG_CYAN         = 36,
        ANSI_FG_GRAY         = 37,
        ANSI_BG_BLACK        = 40,
        ANSI_BG_RED          = 41,
        ANSI_BG_GREEN        = 42,
        ANSI_BG_BROWN        = 43,
        ANSI_BG_BLUE         = 44,
        ANSI_BG_PURPLE       = 45,
        ANSI_BG_CYAN         = 46,
        ANSI_BG_GRAY         = 47,
        ANSI_FG_DARK_GRAY    = 90,
        ANSI_FG_LIGHT_RED    = 91,
        ANSI_FG_LIGHT_GREEN  = 92,
        ANSI_FG_YELLOW       = 93,
        ANSI_FG_LIGHT_BLUE   = 94,
        ANSI_FG_LIGHT_PURPLE = 95,
        ANSI_FG_LIGHT_CYAN   = 96,
        ANSI_FG_WHITE        = 97,
        ANSI_BG_DARK_GRAY    = 100,
        ANSI_BG_LIGHT_RED    = 101,
        ANSI_BG_LIGHT_GREEN  = 102,
        ANSI_BG_YELLOW       = 103,
        ANSI_BG_LIGHT_BLUE   = 104,
        ANSI_BG_LIGHT_PURPLE = 105,
        ANSI_BG_LIGHT_CYAN   = 106,
        ANSI_BG_WHITE        = 107,
    };
}