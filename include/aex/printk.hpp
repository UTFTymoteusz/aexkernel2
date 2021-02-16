#pragma once

#include "aex/utility.hpp"

#include <stdarg.h>

#define PRINTK_INIT "^0"
#define PRINTK_OK "^1"
#define PRINTK_WARN "^2"
#define PRINTK_FAIL "^3"

#define PRINTK_DEBUG(msg) printk("%s:%i: %s\n", __FILE__, __LINE__, msg)
#define PRINTK_DEBUG1(fmt, a) printk("%s:%i: " fmt "\n", __FILE__, __LINE__, a)
#define PRINTK_DEBUG2(fmt, a, b) printk("%s:%i: " fmt "\n", __FILE__, __LINE__, a, b)
#define PRINTK_DEBUG3(fmt, a, b, c) printk("%s:%i: " fmt "\n", __FILE__, __LINE__, a, b, c)

#define PRINTK_DEBUG_WARN(msg) printk(PRINTK_WARN "%s:%i: %s\n", __FILE__, __LINE__, msg)
#define PRINTK_DEBUG_WARN1(fmt, a) printk(PRINTK_WARN "%s:%i: " fmt "\n", __FILE__, __LINE__, a)
#define PRINTK_DEBUG_WARN2(fmt, a, b) \
    printk(PRINTK_WARN "%s:%i: " fmt "\n", __FILE__, __LINE__, a, b)
#define PRINTK_DEBUG_WARN3(fmt, a, b, c) \
    printk(PRINTK_WARN "%s:%i: " fmt "\n", __FILE__, __LINE__, a, b, c)

namespace AEX {
    API void printk(const char* format, ...);
    API void printk(const char* format, va_list args);

    /**
     * Makes printk() not care about spinlocks and ignore prints from other processors.
     **/
    void printk_fault();

    /**
     * Makes printk() care about spinlocks and not ignore prints from other processors.
     **/
    void printk_nofault();
}