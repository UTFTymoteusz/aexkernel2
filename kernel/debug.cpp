#include "aex/debug.hpp"

#include "aex/dev/pci.hpp"
#include "aex/elf.hpp"
#include "aex/errno.hpp"
#include "aex/fs/file.hpp"
#include "aex/mem.hpp"
#include "aex/mem/mmap.hpp"
#include "aex/printk.hpp"
#include "aex/string.hpp"

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

    void load_kernel_symbols(const char* elf_path) {
        auto file_try = FS::File::open(elf_path);
        if (!file_try.has_value)
            kpanic("Failed to load symbols: %s\n", strerror(file_try.error_code));

        auto    file = file_try.value;
        int64_t size = file->seek(0, FS::File::END).value;

        auto mmap_try = Mem::mmap(nullptr, size, Mem::PROT_READ, Mem::MAP_NONE, file, 0);
        if (!mmap_try.has_value)
            kpanic("Failed to mmap the kernel image: %s\n", strerror(mmap_try.error_code));

        file->close();

        void* addr = mmap_try.value;

        load_kernel_symbols_from_memory(addr);

        Mem::munmap(addr, size);
    }

    void load_kernel_symbols_from_memory(void* addr) {
        auto elf = ELF(addr);

        if (!elf.isValid(ELF::bitness_t::BITS64, ELF::endianiness_t::LITTLE, ELF::isa_t::AMD64))
            kpanic("Apparently our own ELF doesn't work on this machine");

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

            auto _symbol = kernel_symbol();

            _symbol.name    = name;
            _symbol.address = symbol.address;

            kernel_symbols.pushBack(_symbol);
        }

        symbols_loaded = true;
    }

    const char* symbol_addr2name(void* addr, int* delta_ret, bool only_kernel) {
        if (kernel_symbols.count() == 0)
            return nullptr;

        if (!only_kernel) {
            const char* ret = module_symbol_addr2name(addr, delta_ret);
            if (ret)
                return ret;
        }

        uint64_t _addr = (uint64_t) addr;
        uint64_t delta = 0x63756e74;

        kernel_symbol match = {};

        for (int i = 0; i < kernel_symbols.count(); i++) {
            auto symbol = kernel_symbols[i];

            if (_addr < symbol.address)
                continue;

            uint64_t new_delta = _addr - symbol.address;
            if (new_delta >= delta)
                continue;

            delta = new_delta;
            match = symbol;
        }

        *delta_ret = delta;

        return match.name;
    }

    const char* symbol_addr2name(void* addr, bool only_kernel) {
        int delta = 0;
        return symbol_addr2name(addr, &delta, only_kernel);
    }

    void* symbol_name2addr(const char* name) {
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
        uint8_t* _addr = (uint8_t*) addr;
        uint16_t cap   = 17;

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
                printk("%p: ", _addr);
            }

            printk("%02x ", *_addr);

            _addr++;
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