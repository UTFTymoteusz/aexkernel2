#pragma once

#include "aex/arch/proc/context.hpp"
#include "aex/spinlock.hpp"
#include "aex/sys/syscall.hpp"
#include "aex/utility.hpp"

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

    template <bool Interruptible>
    class InterruptionGuard;

    struct tss;

    /**
     * The base CPU class that represents a processor in the system and contains some CPU-dependant
     * functionality.
     **/
    class API CPU {
        public:
        enum ipp_type : uint8_t {
            IPP_HALT     = 0,
            IPP_RESHED   = 1,
            IPP_CALL     = 2,
            IPP_PG_FLUSH = 3,
            IPP_PG_INV   = 4,
            IPP_PG_INVM  = 5,
        };

        static constexpr int PAGE_SIZE = 4096;

        struct fault_info {
            Proc::fpu_context fxstate;
            uint64_t r15, r14, r13, r12, r11, r10, r9, r8, rbp, rdi, rsi, rdx, rcx, rbx, rax;

            uint64_t int_no, err;
            uint64_t rip, cs, rflags, rsp, ss;
        } PACKED;

        struct irq_info {
            uint64_t r15, r14, r13, r12, r11, r10, r9, r8, rbp, rdi, rsi, rdx, rcx, rbx, rax;
            uint64_t irq_no;
            uint64_t rip, cs, rflags, rsp, ss;
        } PACKED;

        // Don't change the order of these or the kernel will go boom boom
        int id;
        int apic_id;

        CPU* self;

        // Do not change the order of these or the kernel will go boom boom
        struct {
            Proc::Context* current_context; // 0x08
            Proc::Thread*  current_thread;  // 0x10
            Proc::Thread*  previous_thread; // 0x18

            uint64_t        kernel_stack;  // 0x20
            Sys::syscall_t* syscall_table; // 0x28
        };

        uint64_t irq_stack_start;
        uint64_t reshed_stack_start;

        uint8_t       in_interrupt = 1;
        volatile bool halted;
        bool          should_yield;
        bool          rescheduling;
        tss*          local_tss;

        uint64_t measurement_start_ns;

        uint64_t reshed_fault_stack_start;

        char name[48];
        char vendor[16];

        static InterruptionGuard<true>  interruptsGuard;
        static InterruptionGuard<false> nointerruptsGuard;

        CPU(int id);

        /**
         * Inits the local CPU using the class.
         **/
        void initLocal();

        /**
         * Halts the local CPU.
         **/
        [[noreturn]] static void halt();

        /**
         * Enables interrupts on the local CPU.
         **/
        static void interrupts();

        /**
         * Disables interrupts on the local CPU.
         **/
        static void nointerrupts();

        /**
         * Checks if interrupts are enabled on the local CPU.
         **/
        static bool checkInterrupts();

        /**
         * Waits for an interrupt on the local CPU.
         **/
        static void wait();

        /**
         * Corresponds to the x86 CPUID instruction.
         **/
        static void cpuid(uint32_t code, uint32_t* eax, uint32_t* ebx, uint32_t* ecx,
                          uint32_t* edx);

        static uint8_t  inb(uint16_t m_port);
        static uint16_t inw(uint16_t m_port);
        static uint32_t ind(uint16_t m_port);

        static void outb(uint16_t m_port, uint8_t m_data);
        static void outw(uint16_t m_port, uint16_t m_data);
        static void outd(uint16_t m_port, uint32_t m_data);

        /**
         * Reads a model specific register.
         * @param reg  The register in question.
         * @returns Value read.
         **/
        static uint64_t rdmsr(uint32_t reg);

        /**
         * Writes to a model specific register.
         * @param reg  The register in question.
         * @param data Value to write.
         **/
        static void wrmsr(uint32_t reg, uint64_t data);

        /**
         * Writes to a model specific register.
         * @param reg  The register in question.
         * @param data Value to write.
         **/
        static void wrmsr(uint32_t reg, void* data);

        /**
         * Gets the ID of the executing CPU.
         **/
        static int currentID();

        /**
         * Gets a pointer to the class of the executing CPU.
         **/
        static CPU* current();

        /**
         * Gets a pointer to the current thread.
         **/
        static Proc::Thread* currentThread();

        /**
         * Gets a pointer to the previous thread.
         **/
        static Proc::Thread* previousThread();

        /**
         * Broadcasts a packet to all processors (except the local one, unless you specify
         * otherwise) and IPIs them.
         * @param type Type of the packet.
         * @param data Optional data pointer.
         * @param ignore_self If true, the executing CPU will not receive this packet.
         **/
        static void broadcast(ipp_type type, void* data = nullptr, bool ignore_self = true);

        /**
         * Triple faults the executing processor.
         **/
        static void tripleFault();

        static void breakpoint(int index, size_t addr, uint8_t mode, uint8_t size, bool enabled);

        inline static void flush(void* addr) {
            asm volatile("invlpg [%0]" : : "r"(addr));
        }

        inline static void flush(size_t addr) {
            asm volatile("invlpg [%0]" : : "r"(addr));
        }

        inline static void flush() {
            asm volatile("push rbx; mov rbx, cr3; mov cr3, rbx; pop rbx");
        }

        void printDebug();

        /**
         * Sends a packet to a processor and IPIs it.
         * @param type Type of the packet.
         * @param data Optional data pointer.
         **/
        void send(ipp_type type, void* data = nullptr);

        void swthread(Proc::Thread* thread);

        void pushFmsg(const char* msg);
        void printFmsgs();
        int  countFmsgs();

        private:
        struct ipi_packet {
            ipp_type type;
            void*    data;

            volatile bool ack;
        };

        Spinlock   m_ipi_lock;
        ipi_packet m_ipi_packet;

        char m_fmsgs[16][128];
        int  m_fmsg_ptr;

        void getVendor();
        void getName();

        void _send(ipp_type type, void* data = nullptr);
        void handleIPP();

        friend void ipi_handle();
        friend void AEX::Sys::MCore::init();
    };

    template <bool Interruptible>
    class InterruptionGuardScope {
        public:
        InterruptionGuardScope() {
            m_prevstate = Sys::CPU::checkInterrupts();
        }

        ~InterruptionGuardScope() {
            m_prevstate ? Sys::CPU::interrupts() : Sys::CPU::nointerrupts();
        }

        private:
        bool m_prevstate;
    };

    template <bool Interruptible>
    class InterruptionGuard {
        public:
        InterruptionGuardScope<Interruptible> scope() {
            return InterruptionGuardScope<Interruptible>();
        }

        private:
    };
}