/*
 * apic.hpp: The driver for all APIC stuff
 */
#pragma once

#include "mem/pmem.hpp"

namespace AEX::Sys {
    uint32_t apicRead(int reg);
    void     apicWrite(int reg, uint32_t val);

    class APIC {
      public:
        static void* addr;

        static void map(PMem::phys_addr phys);

        static uint32_t read(int reg);
        static void     write(int reg, uint32_t val);

        static void sendInterrupt(uint8_t dst, uint8_t vector);
        static void sendINIT(uint8_t dst);
        static void sendSIPI(uint8_t dst, uint8_t page);

        static void eoi();
    };

    class IOAPIC {
      public:
        class INTEntry {
          public:
            int id;

            void    setVector(uint8_t vector);
            uint8_t getVector();
        } __attribute((packed));

        static void* addr;

        static uint32_t* addressreg;
        static uint32_t* datareg;

        static INTEntry entries[24];

        static void map(PMem::phys_addr phys);

        static uint32_t read(int reg);
        static void     write(int reg, uint32_t val);
    };
}