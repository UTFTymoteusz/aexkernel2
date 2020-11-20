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

#ifndef ARCH
#define ARCH "inv"
#endif

#ifndef VERSION
#define VERSION "inv"
#endif

#if BIT64
typedef int64_t ssize_t;
#else
typedef int32_t ssize_t;
#endif

namespace AEX {
    template <typename T>
    void swap(T& a, T& b) {
        auto tmp = a;

        a = b;
        b = tmp;
    }

    template <typename T>
    void swap(T& a, T& b, T& c, T& d) {
        swap(a, c);
        swap(b, d);
    }

    template <typename T>
    void swap(T& a, T& b, T& c, T& d, T& e, T& f) {
        swap(a, d);
        swap(b, e);
        swap(c, f);
    }

    template <typename T>
    void swap(T& a, T& b, T& c, T& d, T& e, T& f, T& g, T& h) {
        swap(a, e);
        swap(b, f);
        swap(c, g);
        swap(d, h);
    }
}