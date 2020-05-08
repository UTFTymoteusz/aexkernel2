#include "aex/mem/vmem.hpp"

#include "dev/driver/sata.hpp"
#include "sys/cpu.hpp"

// For some reason math.hpp didn't wanna work
#define min(a, b) (a < b ? a : b)

namespace AEX::Dev::SATA {
    bool SATADevice::init() {
        int  slot   = find_slot();
        auto header = &command_headers[slot];

        header->fis_length = sizeof(AHCI::fis_reg_h2d) / sizeof(uint32_t);
        header->write      = false;
        header->atapi      = false;

        header->phys_region_table_transferred = 0;
        header->phys_region_table_len         = max_prts;

        auto table = &command_tables[slot];
        memset((void*) &(table->fis_reg_h2d_data), '\0', sizeof(AHCI::fis_reg_h2d));

        table->fis_reg_h2d_data.command         = atapi ? 0xA1 : 0xEC;
        table->fis_reg_h2d_data.command_control = true;
        table->fis_reg_h2d_data.fis_type        = 0x27;

        uint8_t buffer[512];
        memset(buffer, '\0', sizeof(buffer));

        fill_prdts(table, buffer, 512);

        while (hba_port->task_file_data & (AHCI::DEV_BUSY | AHCI::DEV_DRQ))
            ;

        start_cmd();
        issue_cmd(slot);

        uint16_t* identify = (uint16_t*) buffer;
        char      flipped_buffer[80];

        memset(flipped_buffer, '\0', sizeof(flipped_buffer));
        memcpy(flipped_buffer, &identify[27], 40);

        for (int i = 0; i < 40; i += 2) {
            char temp = flipped_buffer[i];

            flipped_buffer[i]     = flipped_buffer[i + 1];
            flipped_buffer[i + 1] = temp;
        }

        printk("sata: %s: Model %s\n", this->name, flipped_buffer);

        release_slot(slot);

        return true;
    }

    void SATADevice::start_cmd() {
        hba_port->command_and_status &= ~PxCMD_ST;

        while (hba_port->command_and_status & PxCMD_CR)
            ;

        hba_port->command_and_status |= PxCMD_FRE;
        hba_port->command_and_status |= PxCMD_ST;
    }

    void SATADevice::stop_cmd() {
        auto port = hba_port;

        port->command_and_status &= ~PxCMD_ST;

        while (port->command_and_status & PxCMD_FR || port->command_and_status & PxCMD_CR)
            ;

        port->command_and_status &= ~PxCMD_FRE;
    }

    bool SATADevice::issue_cmd(int slot) {
        hba_port->command_issue = (1 << slot);

        while (true) {
            if (!(hba_port->command_issue & (1 << slot)))
                break;

            if (hba_port->interrupt_status & (1 << 30))
                return false;
        }

        if (hba_port->interrupt_status & (1 << 30))
            return false;

        return true;
    }

    int SATADevice::find_slot() {
        while (true)
            for (int i = 0; i < max_commands; i++)
                if (!(_command_slots & (1 << i)))
                    return i;
    }

    void SATADevice::release_slot(int slot) {
        auto scopeLock = ScopeSpinlock(_lock);

        _command_slots |= (1 << slot);
    }

    void SATADevice::fill_prdts(volatile AHCI::hba_command_table* table, void* dst, size_t len) {
        size_t dsti      = (size_t) dst;
        size_t page_size = Sys::CPU::PAGE_SIZE;

        int index = 0;

        while (len > 0) {
            size_t aligned_next = (dsti + page_size) / page_size * page_size;
            size_t llen         = min(aligned_next - dsti, len);

            table->entries[index].bytes        = llen - 1;
            table->entries[index].data_address = VMem::kernel_pagemap->paddrof((void*) dsti);
            index++;

            dsti += llen;
            len -= llen;
        }
    }
}