#pragma once

#include <stddef.h>
#include <stdint.h>

namespace AEX::Sys {
    extern "C" void ipi_handle();

    /**
     * The base CPU class that represents a processor in the system and contains some CPU-dependant
     * functionality.
     */
    class CPU {
      public:
        /**
         * Halts the local CPU.
         */
        __attribute((noreturn)) static void halt();

        /**
         * Enables interrupts on the local CPU.
         */
        static void interrupts();

        /**
         * Disables interrupts on the local CPU.
         */
        static void nointerrupts();

        /**
         * Checks if interrupts are enabled on the local CPU.
         */
        static bool checkInterrupts();

        /**
         * Waits for an interrupt on the local CPU.
         */
        static void waitForInterrupt();

        /**
         * Corresponds to the x86 CPUID instruction.
         */
        static void cpuid(uint32_t code, uint32_t* eax, uint32_t* ebx, uint32_t* ecx,
                          uint32_t* edx);

        static uint8_t inportb(uint16_t _port);
        static void    outportb(uint16_t _port, uint8_t _data);

        static uint16_t inportw(uint16_t _port);
        static void     outportw(uint16_t _port, uint16_t _data);

        static uint32_t inportd(uint16_t _port);
        static void     outportd(uint16_t _port, uint32_t _data);

      private:
    };
}