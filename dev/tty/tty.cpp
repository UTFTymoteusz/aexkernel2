#include "aex/dev/tty/tty.hpp"

#include "aex/string.hpp"
#include "aex/utility.hpp"

namespace AEX::Dev::TTY {
    char TTY::read() {
        kpanic("Default TTY::read() called");
    }

    /**
     * Reads a line into the specified buffer. Stops on \r or when the buffer is full (counting
     * the null byte at the end).
     **/
    char* TTY::readLine(char*, size_t) {
        NOT_IMPLEMENTED;
    }

    /**
     * Writes a character to the terminal.
     * @param str The character to write out.
     **/
    TTY& TTY::write(char) {
        kpanic("Default TTY::write() called");
    }

    /**
     * Writes a string to the terminal.
     * @param str The string to write out.
     **/
    TTY& TTY::write(const char*) {
        kpanic("Default TTY::write() called");
    }

    bool TTY::text() {
        kpanic("Default TTY::text() called");
    }

    bool TTY::graphics() {
        kpanic("Default TTY::graphics() called");
    }

    tty_info TTY::info() {
        kpanic("Default TTY::info() called");
    }

    void* TTY::output() {
        kpanic("Default TTY::output() called");
    }

    TTY& TTY::color(ansi_color_t _color) {
        char buffer[8];
        snprintf(buffer, sizeof(buffer), "\x1B[%im", _color);
        write(buffer);

        return *this;
    }

    TTY& TTY::operator<<(bool val) {
        write(val ? "true" : "false");
        return *this;
    }

    TTY& TTY::operator<<(char c) {
        write(c);
        return *this;
    }

    TTY& TTY::operator<<(int8_t val) {
        char buffer[8];
        itos(val, 10, buffer);
        write(buffer);

        return *this;
    }

    TTY& TTY::operator<<(uint8_t val) {
        char buffer[8];
        itos(val, 10, buffer);
        write(buffer);

        return *this;
    }

    TTY& TTY::operator<<(int16_t val) {
        char buffer[8];
        itos(val, 10, buffer);
        write(buffer);

        return *this;
    }

    TTY& TTY::operator<<(uint16_t val) {
        char buffer[8];
        itos(val, 10, buffer);
        write(buffer);

        return *this;
    }

    TTY& TTY::operator<<(int32_t val) {
        char buffer[16];
        itos(val, 10, buffer);
        write(buffer);

        return *this;
    }

    TTY& TTY::operator<<(uint32_t val) {
        char buffer[16];
        itos(val, 10, buffer);
        write(buffer);

        return *this;
    }

    TTY& TTY::operator<<(int64_t val) {
        char buffer[32];
        itos(val, 10, buffer);
        write(buffer);

        return *this;
    }

    TTY& TTY::operator<<(uint64_t val) {
        char buffer[32];
        itos(val, 10, buffer);
        write(buffer);

        return *this;
    }

    TTY& TTY::operator<<(void* ptr) {
        char buffer[32];
        itos((size_t) ptr, 16, buffer + 2);

        buffer[0] = '0';
        buffer[1] = 'x';

        write(buffer);

        return *this;
    }

    TTY& TTY::operator<<(const char* str) {
        write(str);
        return *this;
    }

    TTY& TTY::operator<<(ansi_color_t _color) {
        color(_color);
        return *this;
    }
}