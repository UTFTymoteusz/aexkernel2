#pragma once

#include <stdint.h>

namespace AEX::Dev::SATA {
    class AHCI {
      public:
        typedef volatile uint32_t vuint32_t;
        typedef volatile uint64_t vuint64_t;

        struct hba_port_t {
            vuint64_t command_list_address;
            vuint64_t fis_address;

            vuint32_t interrupt_status;
            vuint32_t interrupt_enable;

            vuint32_t command_and_status;
            vuint32_t reserved0;
            vuint32_t task_file_data;
            vuint32_t signature;

            vuint32_t sata_status;
            vuint32_t sata_control;
            vuint32_t sata_error;
            vuint32_t sata_active;
            vuint32_t command_issue;
            vuint32_t sata_notification;

            vuint32_t fis_control;
            vuint32_t reserved1[11];
            vuint32_t vendor_specific[4];
        } __attribute__((packed));

        struct hba_t {
            vuint32_t host_capability;
            vuint32_t global_host_control;
            vuint32_t interrupt_status;
            vuint32_t port_implemented;
            vuint32_t version;

            vuint32_t ccc_ctl;
            vuint32_t ccc_pts;
            vuint32_t em_loc;
            vuint32_t em_ctl;

            vuint32_t host_capability_ext;
            vuint32_t handoff_control;

            uint8_t reserved[0xA0 - 0x2C];
            uint8_t vendor_specific[0x100 - 0xA0];

            hba_port_t ports[];
        } __attribute__((packed));

        struct hba_command_header {
            uint8_t fis_length : 5;
            bool    atapi : 1;
            bool    write : 1;
            bool    prefetchable : 1;

            bool reset : 1;
            bool BIST : 1;
            bool clear_busy : 1;

            uint8_t reserved0 : 1;
            uint8_t port_multiplier_port : 4;

            uint16_t phys_region_table_len;

            volatile uint32_t phys_region_table_transferred;

            uint64_t command_table_addr;

            uint32_t reserved1[4];
        } __attribute__((packed));

        struct phys_region_table_entry {
            uint64_t data_address;
            uint32_t reserved0;

            uint32_t bytes : 22;
            uint32_t reserved1 : 9;
            uint32_t interrupt_on_completion : 1;
        } __attribute__((packed));

        struct hba_command_table {
            uint8_t command_fis[64];
            uint8_t atapi_command[16];
            uint8_t reserved[48];

            phys_region_table_entry entries[];
        } __attribute__((packed));

        struct hba_fis {
            uint8_t idc[256];
        } __attribute__((packed));

        struct device {
            int max_commands   = 1;
            int max_page_burst = 0;

            hba_port_t*         hba_port;
            hba_command_header* command_headers;
            hba_command_table*  command_tables;
            hba_fis*            fis;
        };

        AHCI(void* addr, int index);

      private:
        static constexpr auto POWER_MANAGEMENT_ACTIVE = 0x01;
        static constexpr auto DETECTION_PRESENT       = 0x03;

        static constexpr auto SATA_SIG_ATA   = 0x00000101;
        static constexpr auto SATA_SIG_ATAPI = 0xEB140101;
        static constexpr auto SATA_SIG_SEMB  = 0xC33C0101;
        static constexpr auto SATA_SIG_PM    = 0x96690101;

        hba_t* hba;
        int    index;

        int command_slots = 0;

        void scan_ports();

        void scan_port(hba_port_t* port, int port_index);
    };
}