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

        auto file   = file_try.value;
        auto header = elf_header();

        file->read(&header, sizeof(header));
        if (header.instruction_set != elf_header::isa_t::AMD64)
            kpanic("Apparently our own ELF is not the same arch we're running on (0x%04x)",
                   header.instruction_set);

        uint16_t section_names_index = header.section_header_name_index;

        auto program_headers = new elf_program_header[header.program_header_entry_count];

        file->seek(header.program_header_table_position);
        file->read(program_headers, sizeof(elf_program_header) * header.program_header_entry_count);

        auto section_headers = new elf_section_header[header.section_header_entry_count];

        file->seek(header.section_header_table_position);
        file->read(section_headers, sizeof(elf_section_header) * header.section_header_entry_count);

        auto section_names_header = section_headers[section_names_index];

        char* section_names = new char[section_names_header.size];

        file->seek(section_names_header.file_offset);
        file->read(section_names, section_names_header.size);

        for (int i = 0; i < header.section_header_entry_count; i++) {
            auto section_header = section_headers[i];
            if (section_header.type == elf_section_header::type_t::UNUSED)
                continue;

            char* section_name = section_names + section_header.name_offset;
            if (strcmp(section_name, ".strtab") != 0)
                continue;

            kernel_image_strings = new char[section_header.size];

            file->seek(section_header.file_offset);
            file->read(kernel_image_strings, section_header.size);
        }

        for (int i = 0; i < header.section_header_entry_count; i++) {
            auto section_header = section_headers[i];
            if (section_header.type == elf_section_header::type_t::UNUSED)
                continue;

            char* section_name = section_names + section_header.name_offset;
            if (strcmp(section_name, ".symtab") != 0)
                continue;

            file->seek(section_header.file_offset);

            for (size_t j = 0; j < section_header.size / sizeof(elf_symbol); j++) {
                elf_symbol symbol;
                file->read(&symbol, sizeof(symbol));

                auto _kernel_symbol = kernel_symbol();

                _kernel_symbol.name    = kernel_image_strings + symbol.name_offset;
                _kernel_symbol.address = symbol.address;

                kernel_symbols.pushBack(_kernel_symbol);
            }
        }

        file->close();

        delete program_headers;
        delete section_headers;
        delete section_names;
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