#pragma once

#include <stddef.h>
#include <stdint.h>

namespace AEX::Dev {
    void name_letter_increment(char* buffer, size_t buffer_len, const char* pattern);
    void name_number_increment(char* buffer, size_t buffer_len, const char* pattern);
}