#include "aex/elf.hpp"

namespace AEX {
    ELF::ELF(Mem::SmartPointer<FS::File> file) {
        file->seek(0);
        file->read(&_header, sizeof(_header));

        if (_header.endianiness != endianiness_t::LITTLE)
            return;

        if (_header.bitness == bitness_t::BITS32) {
            int header_name_index = _header.section_header_name_index32;

            file->seek(_header.section_header_table_position32 +
                       header_name_index * sizeof(section_header32));

            section_header32 header_with_names;
            file->read(&header_with_names, sizeof(header_with_names));

            section_names = new char[header_with_names.size];

            file->seek(header_with_names.file_offset);
            file->read((void*) section_names, header_with_names.size);
        }
        else {
            int header_name_index = _header.section_header_name_index;

            file->seek(_header.section_header_table_position +
                       header_name_index * sizeof(section_header64));

            section_header64 header_with_names;
            file->read(&header_with_names, sizeof(header_with_names));

            section_names = new char[header_with_names.size];

            file->seek(header_with_names.file_offset);
            file->read((void*) section_names, header_with_names.size);
        }

        if (_header.bitness == bitness_t::BITS32) {
            int count = _header.program_header_entry_count32;
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
            }
        }
        else {
            int count = _header.program_header_entry_count;
            file->seek(_header.program_header_table_position);

            for (int i = 0; i < count; i++) {
                program_header64 _program_header64;
                file->read(&_program_header64, sizeof(_program_header64));

                program_headers.pushBack((program_header_agnostic) _program_header64);
            }

            int sect_count = _header.section_header_entry_count;
            file->seek(_header.section_header_table_position);

            for (int i = 0; i < sect_count; i++) {
                section_header64 _section_header64;
                file->read(&_section_header64, sizeof(_section_header64));

                section_headers.pushBack(section_header_agnostic(
                    section_names + _section_header64.name_offset, _section_header64));
            }
        }

        for (int i = 0; i < section_headers.count(); i++) {
            auto header = section_headers[i];
            if (strcmp(header.name, ".symtab") != 0)
                continue;

            symbol_table = header;
            break;
        }

        for (int i = 0; i < section_headers.count(); i++) {
            auto header = section_headers[i];
            if (strcmp(header.name, ".strtab") != 0)
                continue;

            strings = new char[header.size];

            file->seek(header.file_offset);
            file->read((void*) strings, header.size);

            string_array_size = header.size;

            break;
        }
    }

    ELF::~ELF() {
        if (section_names)
            delete section_names;
    }

    bool ELF::isValid(bitness_t desired_bitness, endianiness_t desired_endianiness,
                      isa_t desired_isa) {
        if (_header.bitness != desired_bitness || _header.endianiness != desired_endianiness ||
            _header.instruction_set != desired_isa)
            return false;

        return true;
    }
}