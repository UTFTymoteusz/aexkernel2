#pragma once

#include "aex/mem/pmem.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Sys {
    class APIC {
        public:
        static void* addr;

        static void map(PMem::phys_addr phys);

        static uint32_t read(int reg);
        static void     write(int reg, uint32_t val);

        static void init();
        static int  getID();

        static void setupTimer(uint32_t vector);
        static void setupTimer(uint32_t vector, uint32_t initial_count, bool periodic);

        static uint32_t getTimerCounter();
        static uint32_t getTimerInitial();

        static void sendInterrupt(uint8_t dst, uint8_t vector);
        static void sendINIT(uint8_t dst);
        static void sendSIPI(uint8_t dst, uint8_t page);

        static void eoi();
    };

    class IOAPIC {
        public:
        enum irq_mode {
            NORMAL       = 0,
            LOW_PRIORITY = 1,
            SMI          = 2,
            NMI          = 4,
            INIT         = 5,
            EXTERNAL     = 7,
        };

        int irq_base;

        IOAPIC(void* mapped, int base);

        int getIRQAmount();

        void setVector(int irq, uint8_t vector);

        void setMask(int irq, bool mask);

        void setDestination(int irq, uint8_t destination);

        void setMode(int irq, uint8_t mode);

        private:
        volatile uint32_t* addr_reg;
        volatile uint32_t* data_reg;

        uint32_t read(int reg);
        void     write(int reg, uint32_t val);
    } __attribute__((packed));
}