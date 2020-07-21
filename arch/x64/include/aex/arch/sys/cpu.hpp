#pragma once

#include "aex/spinlock.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Proc {
    class Context;
    class Thread;
}

namespace AEX::Sys {
    namespace MCore {
        void init();
    }

    extern "C" void ipi_handle();

    struct tss;

    /**
     * The base CPU class that represents a processor in the system and contains some CPU-dependant
     * functionality.
     */
    class CPU {
        public:
        enum ipp_type : uint8_t {
            IPP_HALT     = 0,
            IPP_RESHED   = 1,
            IPP_CALL     = 2,
            IPP_PG_FLUSH = 3,
            IPP_PG_INV   = 4,
        };

        static constexpr int PAGE_SIZE = 4096;

        struct fault_info {
            uint64_t r15, r14, r13, r12, r11, r10, r9, r8, rbp, rdi, rsi, rdx, rcx, rbx, rax;
            uint64_t int_no, err;
            uint64_t rip, cs, rflags, rsp, ss;
        } __attribute((packed));

        struct irq_info {
            uint64_t r15, r14, r13, r12, r11, r10, r9, r8, rbp, rdi, rsi, rdx, rcx, rbx, rax;
            uint64_t irq_no;
            uint64_t rip, cs, rflags, rsp, ss;
        } __attribute((packed));

        CPU(int id);

        /**
         * Inits the local CPU using the class.
         */
        void initLocal();

        /**
         * Halts the local CPU.
         */
        [[noreturn]] static void halt();

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
         * Reads a model specific register.
         * @param reg  The register in question.
         * @returns Value read.
         */
        static uint64_t rdmsr(uint32_t reg);

        /**
         * Gets the ID of the executing CPU.
         */
        static int getCurrentID();

        /**
         * Gets a pointer to the class of the executing CPU.
         */
        static CPU* getCurrent();

        /**
         * Broadcasts a packet to all processors (except the local one, unless you specify
         * otherwise) and IPIs them.
         * @param type Type of the packet.
         * @param data Optional data pointer.
         * @param ignore_self If true, the executing CPU will not receive this packet.
         */
        static void broadcastPacket(ipp_type type, void* data = nullptr, bool ignore_self = true);

        /**
         * Triple faults the executing processor.
         */
        static void tripleFault();

        /**
         * Sends a packet to a processor and IPIs it.
         * @param type Type of the packet.
         * @param data Optional data pointer.
         */
        void sendPacket(ipp_type type, void* data = nullptr);

        void updateStructures(Proc::Thread* thread);

        // Don't change the order of these or the kernel will go boom boom
        int id;
        int apic_id;

        CPU* self;

        AEX::Proc::Context* currentContext;

        // Safe to change again
        bool    should_yield      = false;
        bool    willingly_yielded = false;
        uint8_t in_interrupt      = 1;

        int                current_tid;
        AEX::Proc::Thread* currentThread;

        uint64_t measurement_start_ns;

        char name[48];

        private:
        struct ipi_packet {
            ipp_type type;
            void*    data;
        };

        Spinlock      _ipi_lock;
        volatile bool _ipi_ack;
        ipi_packet    _ipi_packet;

        tss* _tss;

        void fillAndCleanName();

        void handleIPP();

        friend void ipi_handle();
        friend void AEX::Sys::MCore::init();
    };
}