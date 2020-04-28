#pragma once

#include <stdint.h>

namespace AEX::Sys {
    /**
     * The base CPU class that represents a processor in the system and contains some CPU-dependant
     * functionality.
     */
    class CPU {
      public:
        static constexpr int PAGE_SIZE = 4096;

        struct fault_info {
            uint64_t r15, r14, r13, r12, r11, r10, r9, r8, rbp, rdi, rsi, rdx, rcx, rbx, rax;
            uint64_t int_no, err;
            uint64_t rip, cs, rflags, rsp, ss;
        } __attribute((packed));
        typedef struct fault_info fault_info_t;

        struct irq_info {
            uint64_t r15, r14, r13, r12, r11, r10, r9, r8, rbp, rdi, rsi, rdx, rcx, rbx, rax;
            uint64_t irq_no;
            uint64_t rip, cs, rflags, rsp, ss;
        } __attribute((packed));
        typedef struct irq_info irq_info_t;

        CPU(int id);

        /**
         * Inits the local CPU using the class.
         */
        void initLocal();

        /**
         * Halts the local CPU.
         */
        static void halt();

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

        /**
         * Writes to a model specific register.
         * @param reg  The register in question.
         * @param data Value to write.
         */
        static void wrmsr(uint32_t reg, uint64_t data);

        /**
         * Gets the ID of the executing CPU.
         */
        static int getCurrentCPUID();

        /**
         * Gets a pointer to the class of the executing CPU.
         */
        static CPU* getCurrentCPU();

        int id;
        int apic_id;

        CPU* self;
    };
}