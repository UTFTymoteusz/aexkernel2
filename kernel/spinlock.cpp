#include "aex/spinlock.hpp"

#include "aex/printk.hpp"

#include "sys/cpu.hpp"

namespace AEX {
    char bong = 'A';

    void Spinlock::acquire() {
        while (!__sync_bool_compare_and_swap(&lock, false, true)) {
            asm volatile("pause");

            // Sys::CPU::outportb(0x3F8, bong);
            bong++;
        }

        __sync_synchronize();
    }

    void Spinlock::release() {
        __sync_synchronize();
        __sync_sub_and_fetch(&lock, 1);
    }
}