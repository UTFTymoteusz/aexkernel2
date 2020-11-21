#pragma once

#include "aex/utility.hpp"

#include <stddef.h>
#include <stdint.h>

#if BIT64
typedef int64_t ssize_t;
#else
typedef int32_t ssize_t;
#endif
