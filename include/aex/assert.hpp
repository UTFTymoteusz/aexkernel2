#pragma once

#include "aex/kpanic.hpp"

#define ASSERT(condition)                                                                 \
    ({                                                                                    \
        if (!(condition))                                                                 \
            AEX::kpanic("%s:%i: Assertion failed! (%s)", __FILE__, __LINE__, #condition); \
    })

#define ASSERT_PEDANTIC(condition) ASSERT(condition)