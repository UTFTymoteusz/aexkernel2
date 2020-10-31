#pragma once

#include <stdint.h>

#define PACKED __attribute((packed))
#define WEAK __attribute((weak))
#define UNUSED __attribute((unused))

#define BIT64 INTPTR_MAX == INT64_MAX
#define BIT32 INTPTR_MAX == INT32_MAX
#define BIT16 INTPTR_MAX == INT16_MAX

#define BIG_ENDIAN       \
    (!(union {           \
          uint16_t boi;  \
          uint8_t  test; \
      }){.boi = 1}       \
          .test)

#define LITTLE_ENDIAN (!BIG_ENDIAN)