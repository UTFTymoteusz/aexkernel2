#include "aex/string.hpp"
#include "aex/tty.hpp"

namespace AEX::TTY {
    VTTY& VTTY::operator<<(bool val) {
        write(val ? "true" : "false");
        return *this;
    }

    VTTY& VTTY::operator<<(char c) {
        writeChar(c);
        return *this;
    }

    VTTY& VTTY::operator<<(int8_t val) {
        char buffer[8];
        itos(val, 10, buffer);
        write(buffer);

        return *this;
    }

    VTTY& VTTY::operator<<(uint8_t val) {
        char buffer[8];
        itos(val, 10, buffer);
        write(buffer);

        return *this;
    }

    VTTY& VTTY::operator<<(int16_t val) {
        char buffer[8];
        itos(val, 10, buffer);
        write(buffer);

        return *this;
    }

    VTTY& VTTY::operator<<(uint16_t val) {
        char buffer[8];
        itos(val, 10, buffer);
        write(buffer);

        return *this;
    }

    VTTY& VTTY::operator<<(int32_t val) {
        char buffer[16];
        itos(val, 10, buffer);
        write(buffer);

        return *this;
    }

    VTTY& VTTY::operator<<(uint32_t val) {
        char buffer[16];
        itos(val, 10, buffer);
        write(buffer);

        return *this;
    }

    VTTY& VTTY::operator<<(int64_t val) {
        char buffer[32];
        itos(val, 10, buffer);
        write(buffer);

        return *this;
    }

    VTTY& VTTY::operator<<(uint64_t val) {
        char buffer[32];
        itos(val, 10, buffer);
        write(buffer);

        return *this;
    }

    VTTY& VTTY::operator<<(void* ptr) {
        char buffer[32];
        itos((size_t) ptr, 16, buffer + 2);

        buffer[0] = '0';
        buffer[1] = 'x';

        write(buffer);

        return *this;
    }

    VTTY& VTTY::operator<<(const char* str) {
        write(str);
        return *this;
    }

    VTTY& VTTY::operator<<(ansi_color_t color) {
        setColorANSI(color);
        return *this;
    }
}