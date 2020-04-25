#pragma once

#include "kernel/printk.hpp"
#include "lib/rcparray.hpp"

#include <stdint.h>

// For some reason g++ adds 8 to the offset
#define CURRENT_CPU                                            \
    ((AEX::Sys::CPU*) ({                                       \
        size_t ret = 0;                                        \
        asm volatile("mov %0, qword [gs:-0x08];" : "=r"(ret)); \
        ret;                                                   \
    }))

namespace AEX::Sys {
    class CPU {
      public:
        struct ipi_packet {
            uint8_t type;
            void*   data;
        };
        typedef struct ipi_packet ipi_packet_t;

        enum ipp_type {
            HALT = 0,
        };

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

        static bool isAPICPresent;

        static int PAGE_SIZE;

        /*
         * Runs global initialization procedures.
         */
        static void globalInit();

        /*
         * Halts the executing CPU.
         */
        static void halt();

        /*
         * Enables interrupts on the executing CPU.
         */
        static void interrupts();
        /*
         * Disables interrupts on the executing CPU.
         */
        static void nointerrupts();

        static void cpuid(uint32_t code, uint32_t* eax, uint32_t* ebx, uint32_t* ecx,
                          uint32_t* edx);

        static uint8_t inportb(uint16_t _port);
        static void    outportb(uint16_t _port, uint8_t _data);

        static uint16_t inportw(uint16_t _port);
        static void     outportw(uint16_t _port, uint16_t _data);

        static uint32_t inportd(uint16_t _port);
        static void     outportd(uint16_t _port, uint32_t _data);

        static void wrmsr(uint32_t reg, uint64_t data);

        /*
         * Gets the ID of the executing CPU.
         */
        static int getCurrentCPUID();

        /*
         * Gets a pointer to the class of the executing CPU.
         */
        static CPU* getCurrentCPU();

        /*
         * Broadcasts a packet to all processors (except the executing one, unless you specify
         * otherwise) and IPIs them.
         */
        static void broadcastPacket(uint32_t type, void* data = nullptr, bool ignore_self = true);

        /*
         * Sends a packet to a processor and IPIs it.
         */
        void sendPacket(uint32_t type, void* data = nullptr);

        /*
         * Handles an IPP packet, shouldn't be invoked directly.
         */
        void handle_ipp();

        int id;
        int apic_id;

        CPU* self;

        CPU(int id);

      private:
        void setup_idt();
        void setup_irq();

        void localInit(int id, bool idt = true);

        Spinlock      _ipi_lock;
        volatile bool _ipi_ack;
        ipi_packet_t  _ipi_packet;
    };
}