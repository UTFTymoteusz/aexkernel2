#include "aex/mem.hpp"

#include "usr.hpp"

AEX::optional<size_t> u2k_memcpy(void* dst, const usr_void* src, size_t len) {
    AEX::memcpy(dst, src, len);
    return len;
}

AEX::optional<size_t> k2u_memcpy(usr_void* dst, const void* src, size_t len) {
    AEX::memcpy(dst, src, len);
    return len;
}

AEX::optional<int> usr_strlen(const usr_char* str) {
    int len = 0;

    while (true) {
        auto read_try = usr_read<char>(str++);
        if (!read_try.has_value)
            return {};

        if (read_try.value == '\0')
            break;

        len++;
    }

    return len;
}