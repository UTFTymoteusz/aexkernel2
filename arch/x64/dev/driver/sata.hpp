#pragma once

#include "aex/dev/device.hpp"
#include "aex/spinlock.hpp"

#include "dev/driver/sata/ahci.hpp"

#include <stdint.h>

namespace AEX::Dev::SATA {
    class AHCI;

    enum type_t {
        SATA   = 0,
        SATAPI = 1,
        SEMB   = 2,
        PM     = 3,
    };

    class SATADevice : public Device {
      public:
        static constexpr auto PxCMD_CR  = 0x8000;
        static constexpr auto PxCMD_FR  = 0x4000;
        static constexpr auto PxCMD_FRE = 0x0010;
        static constexpr auto PxCMD_ST  = 0x0001;

        type_t type;
        bool   atapi;

        int max_commands   = 1;
        int max_page_burst = 0;
        int max_prts       = 0;

        AHCI*                     controller;
        AHCI::hba_port_t*         hba_port;
        AHCI::hba_command_header* command_headers;
        AHCI::hba_command_table*  command_tables;
        AHCI::hba_fis*            fis;

        SATADevice(const char* name) : Device(name) {}

        bool init();

      private:
        volatile uint32_t _command_slots;

        Spinlock _lock;

        void start_cmd();
        void stop_cmd();

        bool issue_cmd(int slot);

        int  find_slot();
        void release_slot(int slot);

        void fill_prdts(volatile AHCI::hba_command_table* table, void* dst, size_t len);
    };
}