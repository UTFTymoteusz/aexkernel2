#pragma once

#include "aex/rcparray.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::ACPI {
    enum madt_entry_type {
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
    typedef struct sdt_header sdt_header_t;

    struct rsdp {
        char     signature[8];
        uint8_t  checksum;
        char     oem_id[6];
        uint8_t  revision;
        uint32_t rsdt_address;
    } __attribute((packed));
    typedef struct rsdp rsdp_t;

    struct rsdt {
        sdt_header_t header;
        uint32_t     table_pointers[];
    } __attribute((packed));
    typedef struct rsdt rsdt_t;

    struct xsdp {
        rsdp_t rsdp;

        uint32_t length;
        uint64_t xsdt_address;
        uint8_t  extended_checksum;
        uint8_t  reserved[3];
    } __attribute((packed));
    typedef struct xsdp xsdp_t;

    struct xsdt {
        sdt_header_t header;
        uint64_t     table_pointers[];
    } __attribute((packed));
    typedef struct xsdt xsdt_t;

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
        typedef struct entry entry_t;

        struct lapic {
            entry_t base;

            uint8_t  id;
            uint8_t  apic_id;
            uint32_t flags;
        } __attribute((packed));
        typedef struct lapic lapic_t;

        struct ioapic {
            entry_t base;

            uint8_t  id;
            uint8_t  reserved;
            uint32_t addr;
            uint32_t global_interrupt_base;
        } __attribute((packed));
        typedef struct ioapic ioapic_t;

        struct int_override {
            entry_t base;

            uint8_t  bus_source;
            uint8_t  irq_source;
            uint32_t global_interrupt;
            uint16_t flags;
        } __attribute((packed));
        typedef struct int_override int_override_t;

        struct nmi {
            entry_t base;

            uint8_t  id;
            uint16_t flags;
            uint8_t  num;
        } __attribute((packed));
        typedef struct nmi nmi_t;

        struct addr_override {
            entry_t base;

            uint16_t reserved;
            uint64_t addr;
        } __attribute((packed));
        typedef struct addr_override addr_override_t;

        sdt_header_t header;
        uint32_t     apic_addr;
        uint32_t     flags;

        uint8_t data[];

        template <typename T>
        T findEntry(int type, int index) {
            return (T) findEntry(type, index);
        }

      private:
        void* findEntry(int type, int index);
    } __attribute((packed));

    /*struct madt_entry {
        uint8_t type;
        uint8_t len;
    } __attribute((packed));
    typedef struct madt_entry madt_entry_t;

    struct madt_entry_apic {
        madt_entry_t base;

        uint8_t id;
        uint8_t apic_id;

        uint32_t flags;
    } __attribute((packed));
    typedef struct madt_entry_apic madt_entry_apic_t;*/

    typedef void* table_t;

    extern RCPArray<table_t> tables;

    rsdp_t* find_rsdp();
    xsdp_t* find_xsdp();

    void init();

    bool validate_table(const void* tbl, size_t len);

    table_t* find_table(const char signature[4], int index);

    template <typename T>
    T find_table(const char signature[4], int index) {
        return (T) find_table(signature, index);
    };
}