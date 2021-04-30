#pragma once

#include "aex/spinlock.hpp"
#include "aex/sys/syscall.hpp"

#include <aex/utility.hpp>

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
            uint64_t r15, r14, r13, r12, r11, r10, r9, r8, rbp, rdi, rsi, rdx, rcx, rbx, rax;
            uint64_t int_no, err;
            uint64_t rip, cs, rflags, rsp, ss;
        } __attribute((packed));

        struct irq_info {
            uint64_t r15, r14, r13, r12, r11, r10, r9, r8, rbp, rdi, rsi, rdx, rcx, rbx, rax;
            uint64_t irq_no;
            uint64_t rip, cs, rflags, rsp, ss;
        } __attribute((packed));

        // Don't change the order of these or the kernel will go boom boom
        int id;
        int apic_id;

        CPU* self;

        // Do not change the order of these or the kernel will go boom
        struct {
            Proc::Context* current_context; // 0x08
            Proc::Thread*  current_thread;  // 0x10

            volatile uint64_t unused; // 0x18

            uint64_t        kernel_stack;  // 0x20
            Sys::syscall_t* syscall_table; // 0x28
        };

        uint8_t       in_interrupt = 1;
        volatile bool halted;
        bool          should_yield;
        tss*          local_tss;

        uint64_t measurement_start_ns;

        char name[48];

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

        inline static void flushPg(void* addr) {
            asm volatile("invlpg [%0]" : : "r"(addr));
        }

        inline static void flushPg(size_t addr) {
            asm volatile("invlpg [%0]" : : "r"(addr));
        }

        inline static void flushPg() {
            asm volatile("push rbx; mov rbx, cr3; mov cr3, rbx; pop rbx");
        }

        void printDebug();

        /**
         * Sends a packet to a processor and IPIs it.
         * @param type Type of the packet.
         * @param data Optional data pointer.
         **/
        void send(ipp_type type, void* data = nullptr);

        void update(Proc::Thread* thread);

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

        void getName();

        void _send(ipp_type type, void* data = nullptr);
        void handleIPP();

        friend void ipi_handle();
        friend void AEX::Sys::MCore::init();
    };
}