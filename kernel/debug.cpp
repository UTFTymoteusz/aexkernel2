#include "aex/debug.hpp"

#include "aex/assert.hpp"
#include "aex/elf.hpp"
#include "aex/errno.hpp"
#include "aex/fs/file.hpp"
#include "aex/mem.hpp"
#include "aex/mem/mmap.hpp"
#include "aex/printk.hpp"
#include "aex/proc.hpp"
#include "aex/proc/process.hpp"
#include "aex/string.hpp"
#include "aex/sys/pci.hpp"

#include "boot/mboot.h"
#include "kernel/module.hpp"

using namespace AEX::Mem;

namespace AEX::Debug {
    struct kernel_symbol {
        uint64_t address;
        char*    name;
    };

    bool symbols_loaded = false;
    bool flag           = false;

    Mem::Vector<kernel_symbol> kernel_symbols;
    char*                      kernel_image_strings = nullptr;

    void load_symbols(const char* elf_path) {
        auto file_try = FS::File::open(elf_path, FS::O_RDONLY);
        AEX_ASSERT(file_try);

        auto    file = file_try.value;
        ssize_t size = file->seek(0, FS::File::SEEK_END).value;

        auto mmap_try = Mem::mmap(Proc::Process::kernel(), nullptr, size, Mem::PROT_READ,
                                  Mem::MAP_NONE, file, 0);
        AEX_ASSERT(mmap_try);

        file->close();

        void* addr = mmap_try.value;

        load_symbols(addr);
        Mem::munmap(Proc::Process::kernel(), addr, size);
    }

    void load_symbols(void* addr) {
        auto elf = ELF(addr);
        AEX_ASSERT(elf.isValid(ELF::BIT_64, ELF::EN_LITTLE, ELF::ISA_AMD64));

        elf.loadStrings();
        elf.loadSymbols();

        size_t strings_len = Heap::msize((void*) elf.strings);

        kernel_image_strings = new char[strings_len];
        memcpy(kernel_image_strings, elf.strings, strings_len);

        for (int i = 0; i < elf.symbols.count(); i++) {
            auto symbol = elf.symbols[i];
            if (!symbol.name)
                continue;

            char* name = kernel_image_strings + (symbol.name - elf.strings);
            if (!symbol.address)
                continue;

            auto m_symbol = kernel_symbol();

            m_symbol.name    = name;
            m_symbol.address = symbol.address;

            kernel_symbols.push(m_symbol);
        }

        symbols_loaded = true;
    }

    const char* addr2name(void* addr, int& delta_ret, bool only_kernel) {
        if (kernel_symbols.count() == 0)
            return nullptr;

        if (!only_kernel) {
            const char* ret = module_addr2name(addr, delta_ret);
            if (ret)
                return ret;
        }

        if (addr < (void*) 0xFFFF800000000000) {
            delta_ret = 0;
            return Proc::ready ? "userspace" : "early boot swamp";
        }

        uint64_t m_addr = (uint64_t) addr;
        uint64_t delta  = 0x63756e74;

        kernel_symbol match = {};

        for (int i = 0; i < kernel_symbols.count(); i++) {
            auto symbol = kernel_symbols[i];

            if (m_addr < symbol.address)
                continue;

            uint64_t new_delta = m_addr - symbol.address;
            if (new_delta >= delta || new_delta >= 0x2000)
                continue;

            delta = new_delta;
            match = symbol;
        }

        delta_ret = delta;

        return match.name;
    }

    const char* addr2name(void* addr, bool only_kernel) {
        int delta = 0;
        return addr2name(addr, delta, only_kernel);
    }

    void* name2addr(const char* name) {
        if (kernel_symbols.count() == 0)
            return nullptr;

        kernel_symbol match = {};

        for (int i = 0; i < kernel_symbols.count(); i++) {
            auto symbol = kernel_symbols[i];

            if (strcmp(symbol.name, name) == 0) {
                match = symbol;
                break;
            }
        }

        return (void*) match.address;
    }

    void dump_bytes(void* addr, size_t len) {
        uint8_t* m_addr = (uint8_t*) addr;
        uint16_t cap    = 17;

        while (len) {
            cap--;

            if (cap == 8) {
                printk("  ");
            }
            else if (cap == 0) {
                cap = 16;
                printk("\n");
            }

            if (cap == 16) {
                printk("%p: ", m_addr);
            }

            printk("%02x ", *m_addr);

            m_addr++;
            len--;
        }

        printk("\n");
    }

    char* demangle_name(const char*, char* buffer, size_t) {
        return buffer;
    }
}