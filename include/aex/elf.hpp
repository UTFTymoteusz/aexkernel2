#pragma once

#include <stdint.h>

namespace AEX {
    struct elf_header {
        enum bitness_t : uint8_t {
            BITS32 = 1,
            BITS64 = 2,
        };

        enum endianiness_t : uint8_t {
            LITTLE = 1,
            BIG    = 2,
        };

        enum type_t : uint16_t {
            RELOCATABLE = 1,
            EXECUTABLE  = 2,
            SHARED      = 3,
            CORE        = 4,
        };

        enum isa_t : uint16_t {
            NONE    = 0x00,
            SPARC   = 0x02,
            X86     = 0x03,
            MIPS    = 0x08,
            POWERPC = 0x14,
            ARM     = 0x28,
            SUPERH  = 0x2A,
            IA64    = 0x32,
            AMD64   = 0x3E,
            AARCH64 = 0xB7,
        };

        char          magic[4];
        bitness_t     bitness;
        endianiness_t endianiness;
        uint8_t       header_version;
        uint8_t       os_abi;

        uint8_t padding[8];

        type_t   type;
        isa_t    instruction_set;
        uint32_t version;

        uint64_t entry_position;
        uint64_t program_header_table_position;
        uint64_t section_header_table_position;

        uint32_t flags;
        uint16_t header_size;

        uint16_t program_header_entry_size;
        uint16_t program_header_entry_count;

        uint16_t section_header_entry_size;
        uint16_t section_header_entry_count;

        uint16_t section_header_name_index;
    } __attribute__((packed));

    struct elf_program_header {
        enum type_t : uint32_t { UNUSED = 0, LOAD = 1, DYNAMIC = 2, INTERP = 3, NOTE = 4 };

        enum flags_t : uint32_t {
            EXECUTE = 1,
            WRITE   = 2,
            READ    = 4,
        };

        type_t  type;
        flags_t flags;

        uint64_t file_offset;
        uint64_t address;

        uint64_t undefined;

        uint64_t file_size;
        uint64_t memory_size;

        uint64_t alignment;
    } __attribute__((packed));

    struct elf_section_header {
        enum type_t : uint32_t { UNUSED = 0, PROGRAM = 1, SYMTAB = 2, STRTAB = 3, RELOC = 4 };

        enum flags_t : uint64_t {
            WRITE   = 1,
            ALLOC   = 2,
            EXECUTE = 4,
        };

        uint32_t name_offset;
        type_t   type;
        flags_t  flags;

        uint64_t address;
        uint64_t file_offset;

        uint64_t size;
        uint32_t link;

        uint32_t info;
        uint64_t alignment;
        uint64_t member_size;
    } __attribute__((packed));

    struct elf_symbol {
        uint32_t name_offset;
        uint8_t  info;
        uint8_t  other;

        uint16_t symbol_index;

        uint64_t address;
        uint64_t size;
    } __attribute__((packed));
};