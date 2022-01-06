#include "aex/mem/usr.hpp"

#include "aex/mem.hpp"

namespace AEX::Mem {
    optional<size_t> u2k_memcpy(void* dst, const USER void* src, size_t len) {
        if ((virt_t) src >= kernel_pagemap->vstart)
            return EFAULT;

        if (kernel_pagemap->vstart - (virt_t) src < len)
            return EFAULT;

        if (Proc::setjmp(&Proc::Thread::current()->fault_recovery) == 0) {
            memcpy(dst, src, len);
            Proc::nojmp(&Proc::Thread::current()->fault_recovery);

            return len;
        }
        else
            return EFAULT;
    }

    optional<size_t> k2u_memcpy(USER void* dst, const void* src, size_t len) {
        if ((virt_t) dst >= kernel_pagemap->vstart)
            return EFAULT;

        if (kernel_pagemap->vstart - (virt_t) dst < len)
            return EFAULT;

        if (Proc::setjmp(&Proc::Thread::current()->fault_recovery) == 0) {
            memcpy(dst, src, len);
            Proc::nojmp(&Proc::Thread::current()->fault_recovery);

            return len;
        }
        else
            return EFAULT;
    }

    // TODO: make this not cross into kernelland
    optional<size_t> u_strlen(const USER char* str) {
        if (Proc::setjmp(&Proc::Thread::current()->fault_recovery) == 0) {
            size_t len = strlen(str);
            Proc::nojmp(&Proc::Thread::current()->fault_recovery);

            return len;
        }
        else
            return EFAULT;
    }
}