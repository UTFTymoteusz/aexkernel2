#include "aex/elf.hpp"

#include "aex/mem/mmap.hpp"
#include "aex/printk.hpp"

namespace AEX {
    ELF::ELF(void* addr) {
        _addr = (uint8_t*) addr;

        memcpy(&_header, _addr, sizeof(_header));

        if (_header.endianiness != endianiness_t::LITTLE)
            return;

        if (_header.bitness == bitness_t::BITS32) {
            int header_name_index = _header.section_header_name_index32;

            section_header32 header_with_names;
            memcpy(&header_with_names,
                   _addr + _header.section_header_table_position32 +
                       header_name_index * sizeof(header_with_names),
                   sizeof(header_with_names));

            section_names = new char[header_with_names.size];

            memcpy((void*) section_names, _addr + header_with_names.file_offset,
                   header_with_names.size);
        }
        else {
            int header_name_index = _header.section_header_name_index;

            section_header64 header_with_names;
            memcpy(&header_with_names,
                   _addr + _header.section_header_table_position +
                       header_name_index * sizeof(header_with_names),
                   sizeof(header_with_names));

            section_names = new char[header_with_names.size];

            memcpy((void*) section_names, _addr + header_with_names.file_offset,
                   header_with_names.size);
        }

        if (_header.bitness == bitness_t::BITS32) {
            /*int count = _header.program_header_entry_count32;
            file->seek(_header.program_header_table_position32);

            for (int i = 0; i < count; i++) {
                program_header32 _program_header32;
                file->read(&_program_header32, sizeof(_program_header32));

                program_headers.pushBack((program_header_agnostic) _program_header32);
            }

            int sect_count = _header.section_header_entry_count32;
            file->seek(_header.section_header_table_position32);

            for (int i = 0; i < sect_count; i++) {
                section_header32 _section_header32;
                file->read(&_section_header32, sizeof(_section_header32));

                section_headers.pushBack(section_header_agnostic(
                    section_names + _section_header32.name_offset, _section_header32));
            }*/
        }
        else {
            int count = _header.program_header_entry_count;

            for (int i = 0; i < count; i++) {
                program_header64 _program_header64;
                memcpy(&_program_header64,
                       _addr + _header.program_header_table_position +
                           i * sizeof(_program_header64),
                       sizeof(_program_header64));

                program_headers.pushBack((program_header_agnostic) _program_header64);
            }

            int sect_count = _header.section_header_entry_count;

            for (int i = 0; i < sect_count; i++) {
                section_header64 _section_header64;
                memcpy(&_section_header64,
                       _addr + _header.section_header_table_position +
                           i * sizeof(_section_header64),
                       sizeof(_section_header64));

                section_headers.pushBack(section_header_agnostic(
                    section_names + _section_header64.name_offset, _section_header64));
            }
        }
    }

    ELF::~ELF() {
        if (section_names)
            delete section_names;

        if (strings)
            delete strings;
    }

    bool ELF::isValid(bitness_t desired_bitness, endianiness_t desired_endianiness,
                      isa_t desired_isa) {
        if (_header.bitness != desired_bitness || _header.endianiness != desired_endianiness ||
            _header.instruction_set != desired_isa)
            return false;

        return true;
    }

    void ELF::loadStrings() {
        if (strings)
            return;

        for (int i = 0; i < section_headers.count(); i++) {
            auto header = section_headers[i];
            if (strcmp(header.name, ".strtab") != 0)
                continue;

            strings = new char[header.size];

            memcpy((void*) strings, _addr + header.file_offset, header.size);

            string_array_size = header.size;

            break;
        }
    }

    void ELF::loadSymbols() {
        if (_header.bitness == bitness_t::BITS64)
            loadSymbols64();
    }

    void ELF::loadSymbols64() {
        bool found = false;

        section_header_agnostic symbol_table;

        for (int i = 0; i < section_headers.count(); i++) {
            auto header = section_headers[i];
            if (strcmp(header.name, ".symtab") != 0)
                continue;

            found        = true;
            symbol_table = header;

            break;
        }

        if (!found)
            return;

        loadStrings();

        for (size_t i = 0; i < symbol_table.size; i += sizeof(symbol64)) {
            symbol64 symbol;
            memcpy(&symbol, &_addr[symbol_table.file_offset + i], sizeof(symbol));

            if (symbol.name_offset == 0 || strcmp(strings + symbol.name_offset, "") == 0) {
                auto _symbol = symbol_agnostic();

                _symbol.name          = section_headers[symbol.symbol_index].name;
                _symbol.section_index = symbol.symbol_index;

                symbols.pushBack(_symbol);

                continue;
            }

            auto _symbol = symbol_agnostic();

            _symbol.name          = strings + symbol.name_offset;
            _symbol.address       = symbol.address;
            _symbol.size          = symbol.size;
            _symbol.section_index = symbol.symbol_index;
            _symbol.info          = symbol.info;
            _symbol.other         = symbol.other;

            symbols.pushBack(_symbol);
        }
    }

    void ELF::loadRelocations() {
        if (_header.bitness == bitness_t::BITS64)
            loadRelocations64();
    }

    void ELF::loadRelocations64() {
        for (int i = 0; i < section_headers.count(); i++) {
            auto section_header = section_headers[i];

            if (section_header.type == ELF::sc_type_t::SC_RELOC)
                kpanic("%s: reloc\n", section_header.name);

            if (section_header.type != ELF::sc_type_t::SC_RELOCA)
                continue;

            loadRelocationsFromSection64(section_header);
        }
    }

    void ELF::loadRelocationsFromSection64(section_header_agnostic section) {
        uint32_t symbol_table_id = 0;

        for (int i = 0; i < section_headers.count(); i++) {
            auto header = section_headers[i];
            if (strcmp(header.name, ".symtab") != 0)
                continue;

            symbol_table_id = i;
            break;
        }

        for (size_t i = 0; i < section.size; i += sizeof(relocation_addend64)) {
            relocation_addend64 relocation64;
            memcpy(&relocation64, _addr + section.file_offset + i, sizeof(relocation64));

            uint32_t symbol_id = (relocation64.info >> 32);
            if (symbol_id == 0)
                continue;

            if (section.link != symbol_table_id)
                kpanic("section link doesnt match");

            auto _relocation = relocation();

            _relocation.addr      = relocation64.addr;
            _relocation.arch_info = relocation64.info;
            _relocation.addend    = relocation64.addend;

            _relocation.target_section_id = section.info;
            _relocation.symbol_id         = symbol_id;

            relocations.pushBack(_relocation);
        }
    }
}