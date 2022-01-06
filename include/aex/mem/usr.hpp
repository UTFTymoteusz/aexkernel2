#pragma once

#include "aex/proc/setjmp.hpp"
#include "aex/proc/thread.hpp"
#include "aex/utility.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Mem {
    optional<size_t> u2k_memcpy(void* dst, const USER void* src, size_t len);
    optional<size_t> k2u_memcpy(USER void* dst, const void* src, size_t len);

    optional<size_t> u_strlen(const USER char* str);

    template <typename T>
    optional<T> u_read(const USER void* src) {
        if (Proc::setjmp(&Proc::Thread::current()->fault_recovery) == 0) {
            auto val = *((T*) src);
            Proc::nojmp(&Proc::Thread::current()->fault_recovery);

            return val;
        }
        else
            return EFAULT;
    }

    template <typename T>
    optional<T> u_read(const USER T* src) {
        if (Proc::setjmp(&Proc::Thread::current()->fault_recovery) == 0) {
            auto val = *src;
            Proc::nojmp(&Proc::Thread::current()->fault_recovery);

            return val;
        }
        else
            return EFAULT;
    }

    template <typename T>
    optional<T> u_write(const USER void* dst, T val) {
        if (Proc::setjmp(&Proc::Thread::current()->fault_recovery) == 0) {
            *((T*) dst) = val;
            return val;
        }
        else
            return EFAULT;
    }
}