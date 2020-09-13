#pragma once

#include "aex/kpanic.hpp"

#define AEX_ASSERT(condition)                                                             \
    ({                                                                                    \
        if (!(condition))                                                                 \
            AEX::kpanic("%s:%i: Assertion failed! (%s)", __FILE__, __LINE__, #condition); \
    })

#define AEX_ASSERT_PEDANTIC(condition) AEX_ASSERT(condition)