#include "aex/debug.hpp"

#include "aex/dev/pci.hpp"
#include "aex/elf.hpp"
#include "aex/errno.hpp"
#include "aex/fs/file.hpp"
#include "aex/mem/vector.hpp"
#include "aex/string.hpp"

#include "boot/mboot.h"

extern void main(multiboot_info_t* mbinfo);

namespace AEX::Debug {
    struct kernel_symbol {
        uint64_t address;
        char*    name;
    };

    Mem::Vector<kernel_symbol> kernel_symbols;
    char*                      kernel_image_strings = nullptr;

    void load_kernel_symbols(const char* elf_path) {
        auto file_try = FS::File::open(elf_path);
        if (!file_try.has_value)
            kpanic("Failed to load symbols: %s\n", strerror(file_try.error_code));

        auto file = file_try.value;
        auto elf  = ELF(file);

        if (!elf.isValid(ELF::bitness_t::BITS64, ELF::endianiness_t::LITTLE, ELF::isa_t::AMD64))
            kpanic("Apparently our own ELF doesn't work on this machine");

        kernel_image_strings = new char[elf.string_array_size];
        memcpy(kernel_image_strings, elf.strings, elf.string_array_size);

        for (int i = 0; i < elf.section_headers.count(); i++) {
            auto section_header = elf.section_headers[i];
            if (section_header.type == ELF::sc_type_t::SC_UNUSED)
                continue;

            if (strcmp(section_header.name, ".symtab") != 0)
                continue;

            file->seek(section_header.file_offset);

            for (size_t j = 0; j < section_header.size / sizeof(ELF::symbol); j++) {
                ELF::symbol symbol;
                file->read(&symbol, sizeof(symbol));

                auto _kernel_symbol = kernel_symbol();

                _kernel_symbol.name    = kernel_image_strings + symbol.name_offset;
                _kernel_symbol.address = symbol.address;

                kernel_symbols.pushBack(_kernel_symbol);
            }
        }

        file->close();
    }

    const char* symbol_addr2name(void* addr) {
        if (kernel_symbols.count() == 0)
            return nullptr;

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

        return match.name;
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