#include "aex/debug.hpp"

#include "aex/assert.hpp"
#include "aex/elf.hpp"
#include "aex/errno.hpp"
#include "aex/fs/file.hpp"
#include "aex/mem.hpp"
#include "aex/mem/mmap.hpp"
#include "aex/printk.hpp"
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

    Mem::Vector<kernel_symbol> kernel_symbols;
    char*                      kernel_image_strings = nullptr;

    void load_symbols(const char* elf_path) {
        auto file_try = FS::File::open(elf_path);
        AEX_ASSERT(file_try);

        auto    file = file_try.value;
        int64_t size = file->seek(0, FS::File::SEEK_END).value;

        auto mmap_try = Mem::mmap(nullptr, size, Mem::PROT_READ, Mem::MAP_NONE, file, 0);
        AEX_ASSERT(mmap_try);

        file->close();

        void* addr = mmap_try.value;

        load_symbols(addr);
        Mem::munmap(addr, size);
    }

    void load_symbols(void* addr) {
        auto elf = ELF(addr);

        Debug::dump_bytes((char*) addr - 32, 64);

        printk("0x%x, 0x%x, 0x%x\n", elf.m_header.bitness, elf.m_header.endianiness,
               elf.m_header.instruction_set);
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

    void symbol_debug() {
        for (int i = 0; i < kernel_symbols.count(); i++) {
            auto& symbol = kernel_symbols[i];

            printk("%s - 0x%p\n", symbol.name, symbol.address);

            if (strcmp(symbol.name, "_ZN3AEX8Spinlock7releaseEv") == 0 || i < 5)
                for (volatile size_t i = 0; i < 1223422244; i++)
                    ;
        }
    }

    const char* addr2name(void* addr, int& delta_ret, bool only_kernel) {
        if (kernel_symbols.count() == 0)
            return nullptr;

        if (!only_kernel) {
            const char* ret = module_addr2name(addr, delta_ret);
            if (ret)
                return ret;
        }

        uint64_t m_addr = (uint64_t) addr;
        uint64_t delta  = 0x63756e74;

        kernel_symbol match = {};

        for (int i = 0; i < kernel_symbols.count(); i++) {
            auto symbol = kernel_symbols[i];

            if (m_addr < symbol.address)
                continue;

            uint64_t new_delta = m_addr - symbol.address;
            if (new_delta >= delta)
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

    // Gotta work on this later, cant bother atm
    char* demangle_name(const char* symbol, char* buffer, size_t buffer_len) {
        int  i = 0;
        char parse_buffer[16];

        auto old_buffer = buffer;

        size_t name_buffer_len = 127;
        char   name_buffer[128];

        auto safe_copy = [](char* dst, const char* source, size_t len, size_t* buffer_len) {
            while (*buffer_len && len) {
                *dst = *source;

                dst++;
                source++;

                (*buffer_len)--;
                len--;
            }
        };

        printk("in: %s\n", symbol);

        if (symbol[0] != '_' && symbol[1] != 'Z') {
            strncpy(buffer, symbol, buffer_len);
            printk("unchanged: %s\n", buffer);

            return buffer;
        }

        buffer_len--;

        symbol += 2;

        if (symbol[0] == 'N')
            symbol++;

        char* name_buffer_ptr = name_buffer;

        bool dipped = false;

        while (*symbol != 'E') {
            if (dipped) {
                safe_copy(name_buffer_ptr, "::", 2, &name_buffer_len);
                name_buffer_ptr += 2;
            }

            i = 0;

            while (*symbol >= '0' && *symbol <= '9') {
                parse_buffer[i] = *symbol;
                i++;

                symbol++;
            }

            parse_buffer[i] = '\0';

            int len = stoi<int>(10, parse_buffer);
            safe_copy(name_buffer_ptr, symbol, len, &name_buffer_len);

            name_buffer_ptr += len;
            symbol += len;

            dipped = true;
        }

        // Skip over the E
        symbol++;

        *name_buffer_ptr = '\0';
        printk("out: %s\n", name_buffer);

        return old_buffer;
    }
}