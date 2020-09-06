#pragma once

#include "aex/macros.hpp"
#include "aex/mem.hpp"
#include "aex/string.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::ACPI {
    constexpr auto PROCESSOR_ENABLED = 1 << 0;
    constexpr auto PROCESSOR_ONLINE  = 1 << 1;

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
    } PACKED;

    struct rsdp {
        char     signature[8];
        uint8_t  checksum;
        char     oem_id[6];
        uint8_t  revision;
        uint32_t rsdt_address;
    } PACKED;

    struct rsdt {
        sdt_header header;
        uint32_t   table_pointers[];
    } PACKED;

    struct xsdp {
        rsdp m_rsdp;

        uint32_t length;
        uint64_t xsdt_address;
        uint8_t  extended_checksum;
        uint8_t  reserved[3];
    } PACKED;

    struct xsdt {
        sdt_header header;
        uint64_t   table_pointers[];
    } PACKED;

    struct fadt {
        sdt_header header;
        uint32_t   firmware_ctrl;
        uint32_t   dsdt;

        uint8_t reserved0;

        uint8_t  preffer_power_management_profile;
        uint16_t sci_interrupt;
        uint32_t smi_command_port;

        uint8_t acpi_enable;
        uint8_t acpi_disable;

        uint8_t s4bios_req;
        uint8_t pstate_control;

        uint32_t pm1a_event_block;
        uint32_t pm1b_event_block;
        uint32_t pm1a_control_block;
        uint32_t pm1b_control_block;
        uint32_t pm2_control_block;
        uint32_t pm_timer_block;

        uint32_t gpe0_block;
        uint32_t gpe1_block;

        uint8_t pm1_event_length;
        uint8_t pm1_control_length;
        uint8_t pm2_control_length;
        uint8_t pm_timer_length;

        uint8_t gpe0_length;
        uint8_t gpe1_length;
        uint8_t gpe1_base;

        uint8_t c_state_control;

        uint16_t worst_c2_latency;
        uint16_t worst_c3_latency;

        uint16_t flush_size;
        uint16_t flush_stride;

        uint8_t duty_offset;
        uint8_t duty_width;

        uint8_t day_alarm;
        uint8_t month_alarm;
        uint8_t century;

        uint16_t boot_arch_flags;

        uint8_t  reserved1;
        uint32_t fixed_flags;
    } PACKED;

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

        private:
        void* findEntry(int type, int index);
    } PACKED;

    struct acpi_table {};

    extern Mem::Vector<acpi_table*> tables;
    extern uint8_t                  revision;

    rsdp* find_rsdp();
    xsdp* find_xsdp();

    void init();

    bool validate_table(const void* tbl, size_t len);

    acpi_table* find_table(const char signature[4], int index);
}