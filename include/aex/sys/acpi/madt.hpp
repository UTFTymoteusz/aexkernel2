#pragma once

#include "aex/sys/acpi/sdt.hpp"
#include "aex/utility.hpp"

#include <stdint.h>

namespace AEX::Sys::ACPI {
    constexpr auto PROCESSOR_ENABLED = 1 << 0;
    constexpr auto PROCESSOR_ONLINE  = 1 << 1;

    struct API madt {
        enum entry_type {
            LAPIC      = 0,
            IOAPIC     = 1,
            IRQ_SOURCE = 2,
            NMI        = 4,
            LAPIC_ADDR = 5,
        };

        struct entry {
            uint8_t type;
            uint8_t len;
        } PACKED;

        struct lapic {
            entry base;

            uint8_t  id;
            uint8_t  apic_id;
            uint32_t flags;

            bool canStart() {
                return flags & PROCESSOR_ENABLED || flags & PROCESSOR_ONLINE;
            }
        } PACKED;

        struct ioapic {
            entry base;

            uint8_t  id;
            uint8_t  reserved;
            uint32_t addr;
            uint32_t global_interrupt_base;
        } PACKED;

        struct int_override {
            entry base;

            uint8_t  bus_source;
            uint8_t  irq_source;
            uint32_t global_interrupt;
            uint16_t flags;
        } PACKED;

        struct nmi {
            entry base;

            uint8_t  id;
            uint16_t flags;
            uint8_t  num;
        } PACKED;

        struct addr_override {
            entry base;

            uint16_t reserved;
            uint64_t addr;
        } PACKED;

        sdt_header header;
        uint32_t   apic_addr;
        uint32_t   flags;

        uint8_t data[];

        template <typename T>
        T findEntry(int type, int index) {
            return (T) findEntry(type, index);
        }

        void* findEntry(int type, int index);
    } PACKED;
}