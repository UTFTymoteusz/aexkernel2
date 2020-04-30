#pragma once

#include "aex/rcparray.hpp"
#include "aex/string.hpp"

#include <stddef.h>
#include <stdint.h>

#define PROCESSOR_ENABLED 1 << 0
#define PROCESSOR_ONLINE 1 << 1

namespace AEX::ACPI {
    enum madt_entryype {
        APIC = 0,
    };

    struct sdt_header {
        char signature[4];

        uint32_t length;
        uint8_t  revision;
        uint8_t  checksum;

        char oem_id[6];
        char oem_tbl_id[8];

        uint32_t oem_revision;
        uint32_t creator_id;
        uint32_t creator_revision;
    } __attribute((packed));

    struct rsdp {
        char     signature[8];
        uint8_t  checksum;
        char     oem_id[6];
        uint8_t  revision;
        uint32_t rsdt_address;
    } __attribute((packed));

    struct rsdt {
        sdt_header header;
        uint32_t   table_pointers[];
    } __attribute((packed));

    struct xsdp {
        rsdp _rsdp;

        uint32_t length;
        uint64_t xsdt_address;
        uint8_t  extended_checksum;
        uint8_t  reserved[3];
    } __attribute((packed));

    struct xsdt {
        sdt_header header;
        uint64_t   table_pointers[];
    } __attribute((packed));

    class MADT {
      public:
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
        } __attribute((packed));

        struct lapic {
            entry base;

            uint8_t  id;
            uint8_t  apic_id;
            uint32_t flags;

            bool canStart() {
                return flags & PROCESSOR_ENABLED || flags & PROCESSOR_ONLINE;
            }
        } __attribute((packed));

        struct ioapic {
            entry base;

            uint8_t  id;
            uint8_t  reserved;
            uint32_t addr;
            uint32_t global_interrupt_base;
        } __attribute((packed));

        struct int_override {
            entry base;

            uint8_t  bus_source;
            uint8_t  irq_source;
            uint32_t global_interrupt;
            uint16_t flags;
        } __attribute((packed));

        struct nmi {
            entry base;

            uint8_t  id;
            uint16_t flags;
            uint8_t  num;
        } __attribute((packed));

        struct addr_override {
            entry base;

            uint16_t reserved;
            uint64_t addr;
        } __attribute((packed));

        sdt_header header;
        uint32_t   apic_addr;
        uint32_t   flags;

        uint8_t data[];

        template <typename T>
        T findEntry(int type, int index) {
            return (T) findEntry(type, index);
        }

      private:
        void* findEntry(int type, int index);
    } __attribute((packed));

    typedef void* acpi_table;

    extern RCPArray<acpi_table> tables;

    rsdp* find_rsdp();
    xsdp* find_xsdp();

    void init();

    bool validate_table(const void* tbl, size_t len);

    RCPArray<acpi_table>::Pointer find_table(const char signature[4], int index);
}