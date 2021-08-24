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
#define ARCH "inv43"
#endif

#ifndef VERSION
#define VERSION "00.00.0000.00000"
#endif

namespace AEX {
    [[noreturn]] API void kpanic(const char* format, ...);
}

#define NOT_IMPLEMENTED kpanic("%s:%i: Not implemented", __FILE__, __LINE__)
#define BROKEN kpanic("%s:%i: %s() is broken", __FILE__, __LINE__, __func__)
#define FUNC_NOT_IMPLEMENTED \
    { kpanic("%s:%i: %s(): Not implemented", __FILE__, __LINE__, __func__); }

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
#define ENSURE_NERR(cond)      \
    ({                         \
        auto res = cond;       \
        if (res != AEX::ENONE) \
            return res;        \
                               \
        res;                   \
    })
#define ENSURE(cond) ENSURE_R((cond), AEX::EINVAL)
#define ENSURE_FL(flags, mask) ENSURE_R(!(flags & ~mask), AEX::EINVAL)

#define _CONCAT(a, b) a##b
#define CONCAT(a, b) _CONCAT(a, b)
#define SCOPE(x) auto CONCAT(scope, __LINE__) = x.scope()

#define using(lock)          \
    for (int scbong = ({     \
             lock.acquire(); \
             0;              \
         });                 \
         scbong < 1; ({      \
             lock.release(); \
             scbong++;       \
         }))

#define finally(code) \
    while (({         \
        (code);       \
        false;        \
    }))               \
        ;

struct scopebong {
    int  i;
    bool state;
};

#define interruptible(int)                                  \
    for (scopebong intbong = ({                             \
             bool state = AEX::Sys::CPU::checkInterrupts(); \
             if (int)                                       \
                 AEX::Sys::CPU::interrupts();               \
             else                                           \
                 AEX::Sys::CPU::nointerrupts();             \
                                                            \
             scopebong{                                     \
                 .i     = 0,                                \
                 .state = state,                            \
             };                                             \
         });                                                \
         intbong.i < 1; ({                                  \
             if (!intbong.state)                            \
                 AEX::Sys::CPU::nointerrupts();             \
             else                                           \
                 AEX::Sys::CPU::interrupts();               \
                                                            \
             intbong.i++;                                   \
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

    namespace {
        template <typename T>
        constexpr auto bor(T a) {
            return a;
        }

        template <typename T, typename U>
        constexpr auto bor(T a, U b) {
            return a | b;
        }

        template <typename T, typename U, typename... V>
        constexpr auto bor(T a, U b, V... c) {
            return a | bor(b, c...);
        }
    }

    template <typename T, typename... U>
    constexpr bool flagmask(T val, U... bits) {
        return val & ~bor(bits...);
    }

    template <typename T, typename... U>
    constexpr bool flagset(T val, U... bits) {
        auto flags = bor(bits...);
        return (val & flags) == flags;
    }
}