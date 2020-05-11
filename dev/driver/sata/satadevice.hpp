#pragma once

#include "aex/dev/tree/device.hpp"

#include "dev/driver/sata/ahci.hpp"

namespace AEX::Dev::SATA {
    class AHCI;

    enum type_t {
        SATA   = 0,
        SATAPI = 1,
        SEMB   = 2,
        PM     = 3,
    };

    class SATADevice : public Tree::Device {
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

        uint64_t sector_count;

        AHCI*                     controller;
        AHCI::hba_port_t*         hba_port;
        AHCI::hba_command_header* command_headers;
        AHCI::hba_command_table*  command_tables;
        AHCI::hba_fis*            fis;

        Spinlock _lock;

        SATADevice(const char* name) : Device(name) {}

        bool init();

        void startCMD();
        void stopCMD();

        bool issueCMD(int slot);

        int  findSlot();
        void releaseSlot(int slot);

        void fillPRDTs(volatile AHCI::hba_command_table* table, void* dst, size_t len);

        void scsiPacket(uint8_t* packet, void* buffer, int len);

      private:
        volatile uint32_t _command_slots;

        AHCI::hba_command_header* getHeader(int slot);
        AHCI::hba_command_table*  getTable(int slot);
    };
}