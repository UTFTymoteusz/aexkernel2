#pragma once

#define API __attribute__((visibility("default")))

#include "aex/errno.hpp"

#define PACKED __attribute((packed))
#define WEAK __attribute((weak))
#define UNUSED __attribute((unused))
#define FALLTHROUGH __attribute__((fallthrough))

#define fall FALLTHROUGH

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

namespace AEX {
    [[noreturn]] API void kpanic(const char* format, ...);
}

#define NOT_IMPLEMENTED kpanic("%s:%i: %s\n", __FILE__, __LINE__, "Not implemented")

#define ENSURE_R(cond, err) \
    ({                      \
        auto ret = cond;    \
        if (!(ret))         \
            return err;     \
                            \
        ret;                \
    })
#define ENSURE_OPT(opt)       \
    ({                        \
        auto res = opt;       \
        if (!res)             \
            return res.error; \
                              \
        res.value;            \
    })
#define ENSURE(cond) ENSURE_R((cond), AEX::EINVAL)
#define ENSURE_FL(flags, mask) ENSURE_R(!(flags & ~mask), AEX::EINVAL)

#define CONCAT(a, b) a##b
#define SCOPE(x) auto CONCAT(scope, __LINE__) = x.scope()

#define using(lock)      \
    if (({               \
            SCOPE(lock); \
            true;        \
        }))

namespace AEX {
    template <typename T>
    inline void swap(T& a, T& b) {
        auto tmp = a;

        a = b;
        b = tmp;
    }

    template <typename T>
    inline void swap(T& a, T& b, T& c, T& d) {
        swap(a, c);
        swap(b, d);
    }

    template <typename T>
    inline void swap(T& a, T& b, T& c, T& d, T& e, T& f) {
        swap(a, d);
        swap(b, e);
        swap(c, f);
    }

    template <typename T>
    inline void swap(T& a, T& b, T& c, T& d, T& e, T& f, T& g, T& h) {
        swap(a, e);
        swap(b, f);
        swap(c, g);
        swap(d, h);
    }
}