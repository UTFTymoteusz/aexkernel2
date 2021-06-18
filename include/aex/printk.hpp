#pragma once

#include "aex/utility.hpp"

#include <stdarg.h>

#define INIT "%0^"
#define OK "%1^"
#define WARN "%2^"
#define FAIL "%3^"

#define printkd(cond, format, args...) \
    ({                                 \
        if (cond)                      \
            printk(format, ##args);    \
    })

#define PTKD_FS false
#define PTKD_EXEC false
#define PTKD_UFAULT true
#define PTKD_PROC false
#define PTKD_IPC true
#define PTKD_MMAP true

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