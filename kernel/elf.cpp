#include "aex/elf.hpp"

#include "aex/assert.hpp"
#include "aex/mem/mmap.hpp"

namespace AEX {
    ELF::ELF(void* addr) {
        m_addr = (uint8_t*) addr;

        memcpy(&m_header, m_addr, sizeof(m_header));

        if (m_header.endianiness != endianiness_t::EN_LITTLE)
            return;

        if (m_header.bitness == BIT_32) {
            int header_name_index = m_header.section_header_name_index32;

            section_header32 header_with_names;
            memcpy(&header_with_names,
                   m_addr + m_header.section_header_table_position32 +
                       header_name_index * sizeof(header_with_names),
                   sizeof(header_with_names));

            section_names = new char[header_with_names.size];

            memcpy((void*) section_names, m_addr + header_with_names.file_offset,
                   header_with_names.size);

            entry = (void*) (size_t) m_header.entry_position32;
        }
        else {
            int header_name_index = m_header.section_header_name_index;

            section_header64 header_with_names;
            memcpy(&header_with_names,
                   m_addr + m_header.section_header_table_position +
                       header_name_index * sizeof(header_with_names),
                   sizeof(header_with_names));

            section_names = new char[header_with_names.size];

            memcpy((void*) section_names, m_addr + header_with_names.file_offset,
                   header_with_names.size);

            entry = (void*) (size_t) m_header.entry_position;
        }

        if (m_header.bitness == BIT_32) {
            /*int count = m_header.program_header_entry_count32;
            file->seek(m_header.program_header_table_position32);

            for (int i = 0; i < count; i++) {
                program_header32 m_program_header32;
                file->read(&m_program_header32, sizeof(m_program_header32));

                program_headers.push((program_header_agn) m_program_header32);
            }

            int sect_count = m_header.section_header_entry_count32;
            file->seek(m_header.section_header_table_position32);

            for (int i = 0; i < sect_count; i++) {
                section_header32 m_section_header32;
                file->read(&m_section_header32, sizeof(m_section_header32));

                section_headers.push(section_header_agn(
                    section_names + m_section_header32.name_offset, m_section_header32));
            }*/
        }
        else {
            int count = m_header.program_header_entry_count;

            for (int i = 0; i < count; i++) {
                program_header64 m_program_header64;
                memcpy(&m_program_header64,
                       m_addr + m_header.program_header_table_position +
                           i * sizeof(m_program_header64),
                       sizeof(m_program_header64));

                program_headers.push((program_header_agn) m_program_header64);
            }

            int sect_count = m_header.section_header_entry_count;

            for (int i = 0; i < sect_count; i++) {
                section_header64 m_section_header64;
                memcpy(&m_section_header64,
                       m_addr + m_header.section_header_table_position +
                           i * sizeof(m_section_header64),
                       sizeof(m_section_header64));

                section_headers.push(section_header_agn(
                    section_names + m_section_header64.name_offset, m_section_header64));
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
        if (m_header.bitness != desired_bitness || m_header.endianiness != desired_endianiness ||
            m_header.instruction_set != desired_isa)
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

            memcpy((void*) strings, m_addr + header.file_offset, header.size);

            string_array_size = header.size;

            break;
        }
    }

    void ELF::loadSymbols() {
        if (m_header.bitness == BIT_64)
            loadSymbols64();
    }

    void ELF::loadSymbols64() {
        bool found = false;

        section_header_agn symbol_table;

        for (auto& header : section_headers) {
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
            memcpy(&symbol, &m_addr[symbol_table.file_offset + i], sizeof(symbol));

            if (symbol.name_offset == 0 || strcmp(strings + symbol.name_offset, "") == 0) {
                auto m_symbol = symbol_agn();

                m_symbol.name          = section_headers[symbol.symbol_index].name;
                m_symbol.section_index = symbol.symbol_index;

                symbols.push(m_symbol);

                continue;
            }

            auto m_symbol = symbol_agn();

            m_symbol.name          = strings + symbol.name_offset;
            m_symbol.address       = symbol.address;
            m_symbol.size          = symbol.size;
            m_symbol.section_index = symbol.symbol_index;
            m_symbol.info          = symbol.info;
            m_symbol.other         = symbol.other;

            symbols.push(m_symbol);
        }
    }

    void ELF::loadRelocations() {
        if (m_header.bitness == BIT_64)
            loadRelocations64();
    }

    void ELF::loadRelocations64() {
        for (auto& header : section_headers) {
            ASSERT(header.type != ELF::sc_type_t::SC_RELOC);
            if (header.type != ELF::sc_type_t::SC_RELOCA)
                continue;

            loadRelocationsFromSection64(header);
        }
    }

    void ELF::loadRelocationsFromSection64(section_header_agn section) {
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
            memcpy(&relocation64, m_addr + section.file_offset + i, sizeof(relocation64));

            uint32_t symbol_id = (relocation64.info >> 32);
            if (symbol_id == 0)
                continue;

            ASSERT(section.link == symbol_table_id);

            relocations.push({
                .addr              = relocation64.addr,
                .arch_info         = relocation64.info,
                .addend            = relocation64.addend,
                .target_section_id = section.info,
                .symbol_id         = symbol_id,
            });
        }
    }
}