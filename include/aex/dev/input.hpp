#pragma once

namespace AEX::Dev {
    // https://source.android.com/devices/input/keyboard-devices uwu
    // Polish programmers layout because 214 is retarded
    enum hid_keycode_t {
        KEY_A          = 0x04,
        KEY_B          = 0x05,
        KEY_C          = 0x06,
        KEY_D          = 0x07,
        KEY_E          = 0x08,
        KEY_F          = 0x09,
        KEY_G          = 0x0A,
        KEY_H          = 0x0B,
        KEY_I          = 0x0C,
        KEY_J          = 0x0D,
        KEY_K          = 0x0E,
        KEY_L          = 0x0F,
        KEY_M          = 0x10,
        KEY_N          = 0x11,
        KEY_O          = 0x12,
        KEY_P          = 0x13,
        KEY_Q          = 0x14,
        KEY_R          = 0x15,
        KEY_S          = 0x16,
        KEY_T          = 0x17,
        KEY_U          = 0x18,
        KEY_V          = 0x19,
        KEY_W          = 0x1A,
        KEY_X          = 0x1B,
        KEY_Y          = 0x1C,
        KEY_Z          = 0x1D,
        KEY_1          = 0x1E,
        KEY_2          = 0x1F,
        KEY_3          = 0x20,
        KEY_4          = 0x21,
        KEY_5          = 0x22,
        KEY_6          = 0x23,
        KEY_7          = 0x24,
        KEY_8          = 0x25,
        KEY_9          = 0x26,
        KEY_0          = 0x27,
        KEY_RETURN     = 0x28,
        KEY_ESCAPE     = 0x29,
        KEY_BACKSPACE  = 0x2A,
        KEY_TAB        = 0x2B,
        KEY_SPACE      = 0x2C,
        KEY_MINUS      = 0x2D,
        KEY_EQUAL      = 0x2E,
        KEY_LEFTBRACE  = 0x2F,
        KEY_RIGHTBRACE = 0x30,
        KEY_BACKSLASH  = 0x31,
        KEY_BACKSLASH2 = 0x32,
        KEY_SEMICOLON  = 0x33,
        KEY_APOSTROPHE = 0x34,
        KEY_GRAVE      = 0x35,
        KEY_COMMA      = 0x36,
        KEY_DOT        = 0x37,
        KEY_SLASH      = 0x38,
        KEY_CAPSLOCK   = 0x39,
    }
}
