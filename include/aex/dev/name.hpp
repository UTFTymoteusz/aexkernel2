#pragma once

#include "aex/utility.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Dev {
    API void name_letter_increment(char* buffer, size_t buffer_len, const char* pattern);
    API void name_number_increment(char* buffer, size_t buffer_len, const char* pattern);
}