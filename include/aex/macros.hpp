#pragma once

#define PACKED __attribute((packed))
#define WEAK __attribute((weak))

#define BIT64 INTPTR_MAX == INT64_MAX
#define BIT32 INTPTR_MAX == INT32_MAX
#define BIT16 INTPTR_MAX == INT16_MAX