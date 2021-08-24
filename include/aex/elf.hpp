#pragma once

#include "aex/fs/file.hpp"
#include "aex/mem.hpp"
#include "aex/mem/vector.hpp"
#include "aex/utility.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX {
    class API ELF {
        public:
        enum relocation_type {
            R_X86_64_NONE  = 0,
            R_X86_64_64    = 1,
            R_X86_64_PC32  = 2,
            R_X86_64_GOT32 = 3,
            R_X86_64_32S   = 11,
        };

        enum bitness_t : uint8_t {
            BIT_32 = 1,
            BIT_64 = 2,
        };

        enum endianiness_t : uint8_t {
            EN_LITTLE = 1,
            EN_BIG    = 2,
        };

        enum isa_t : uint16_t {
            ISA_NONE    = 0x00,
            ISA_SPARC   = 0x02,
            ISA_X86     = 0x03,
            ISA_MIPS    = 0x08,
            ISA_POWERPC = 0x14,
            ISA_ARM     = 0x28,
            ISA_SUPERH  = 0x2A,
            ISA_IA64    = 0x32,
            ISA_AMD64   = 0x3E,
            ISA_AARCH64 = 0xB7,
        };

        enum ph_type_t : uint32_t {
            PH_UNUSED  = 0,
            PH_LOAD    = 1,
            PH_DYNAMIC = 2,
            PH_INTERP  = 3,
            PH_NOTE    = 4,
            PH_TLS     = 7,
        };

        enum ph_flags_t : uint32_t {
            PH_EXECUTE = 1,
            PH_WRITE   = 2,
            PH_READ    = 4,
        };

        enum sc_type_t : uint32_t {
            SC_UNUSED  = 0,
            SC_PROGRAM = 1,
            SC_SYMTAB  = 2,
            SC_STRTAB  = 3,
            SC_RELOCA  = 4,
            SC_NO_DATA = 8,
            SC_RELOC   = 9,
        };

        enum sc_flags_t : uint32_t {
            SC_WRITE   = 1,
            SC_ALLOC   = 2,
            SC_EXECUTE = 4,
        };

        enum sym_binding : uint8_t {
            SB_LOCAL  = 0,
            SB_GLOBAL = 1,
            SB_       = 2,
        };

        enum sym_special {
            SHN_ABS = 0xFFF1,
        };

        struct header {
            enum type_t : uint16_t {
                HDR_RELOCATABLE = 1,
                HDR_EXECUTABLE  = 2,
                HDR_SHARED      = 3,
                HDR_CORE        = 4,
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

            union {
                struct {
                    uint32_t entry_position32;
                    uint32_t program_header_table_position32;
                    uint32_t section_header_table_position32;

                    uint32_t flags32;
                    uint16_t header_size32;

                    uint16_t program_header_entry_size32;
                    uint16_t program_header_entry_count32;

                    uint16_t section_header_entry_size32;
                    uint16_t section_header_entry_count32;

                    uint16_t section_header_name_index32;
                };

                struct {
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
                };
            };
        } PACKED;

        struct program_header32 {
            ph_type_t type;

            uint32_t file_offset;
            uint32_t address;

            uint32_t undefined;

            uint32_t file_size;
            uint32_t memory_size;

            ph_flags_t flags;

            uint32_t alignment;
        } PACKED;

        struct program_header64 {
            ph_type_t  type;
            ph_flags_t flags;

            uint64_t file_offset;
            uint64_t address;

            uint64_t undefined;

            uint64_t file_size;
            uint64_t memory_size;

            uint64_t alignment;
        } PACKED;

        struct program_header_agn {
            ph_type_t type;

            uint64_t file_offset;
            uint64_t address;

            uint64_t file_size;
            uint64_t memory_size;

            ph_flags_t flags;

            uint64_t alignment;

            union {
                program_header32 base32;
                program_header64 base64;
            };

            program_header_agn() {}

            program_header_agn(program_header32 header) {
                file_offset = header.file_offset;
                address     = header.address;

                file_size   = header.file_size;
                memory_size = header.memory_size;

                flags     = header.flags;
                alignment = header.alignment;
                type      = header.type;

                base32 = header;
            }

            program_header_agn(program_header64 header) {
                file_offset = header.file_offset;
                address     = header.address;

                file_size   = header.file_size;
                memory_size = header.memory_size;

                flags     = header.flags;
                alignment = header.alignment;
                type      = header.type;

                base64 = header;
            }
        };

        struct section_header32 {
            uint32_t name_offset;

            sc_type_t  type;
            sc_flags_t flags;

            uint32_t address;
            uint32_t file_offset;

            uint32_t size;
            uint32_t link;

            uint32_t info;
            uint32_t alignment;
            uint32_t member_size;
        } PACKED;

        struct section_header64 {
            uint32_t name_offset;

            sc_type_t  type;
            sc_flags_t flags;
            uint32_t   padding;

            uint64_t address;
            uint64_t file_offset;

            uint64_t size;
            uint32_t link;

            uint32_t info;
            uint64_t alignment;
            uint64_t member_size;
        } PACKED;

        struct section_header_agn {
            const char* name;
            char*       strings = nullptr;

            bitness_t bitness;

            sc_type_t  type;
            sc_flags_t flags;
            uint32_t   padding;

            uint64_t address;
            uint64_t file_offset;

            uint64_t size;
            uint32_t link;

            uint32_t info;
            uint64_t alignment;
            uint64_t member_size;

            union {
                section_header32 base32;
                section_header64 base64;
            };

            section_header_agn() {}

            section_header_agn(const char* name, section_header32 header) {
                this->name    = name;
                this->bitness = BIT_32;

                type  = header.type;
                flags = header.flags;

                address     = header.address;
                file_offset = header.file_offset;

                size = header.size;
                link = header.link;

                info        = header.info;
                alignment   = header.alignment;
                member_size = header.member_size;

                base32 = header;
            }

            section_header_agn(const char* name, section_header64 header) {
                this->name    = name;
                this->bitness = BIT_64;

                type  = header.type;
                flags = header.flags;

                address     = header.address;
                file_offset = header.file_offset;

                size = header.size;
                link = header.link;

                info        = header.info;
                alignment   = header.alignment;
                member_size = header.member_size;

                base64 = header;
            }
        };


        struct symbol64 {
            uint32_t name_offset;
            uint8_t  info;
            uint8_t  other;

            uint16_t symbol_index = 0;

            uint64_t address;
            uint64_t size;
        } PACKED;

        struct symbol_agn {
            const char* name;

            uint16_t section_index = 0;

            uint8_t info;
            uint8_t other;

            uint64_t address;
            uint64_t size;
        } PACKED;

        struct relocation64 {
            uint64_t addr;
            uint64_t info;
        } PACKED;

        struct relocation_addend64 {
            uint64_t addr;
            uint64_t info;
            int64_t  addend;
        } PACKED;

        struct relocation {
            uint64_t addr;
            uint64_t arch_info;
            int64_t  addend;

            uint32_t target_section_id;
            uint32_t symbol_id;
        } PACKED;

        Mem::Vector<program_header_agn> program_headers;
        Mem::Vector<section_header_agn> section_headers;

        Mem::Vector<symbol_agn, 256> symbols;
        Mem::Vector<relocation, 256> relocations;

        const char* section_names = nullptr;
        const char* strings       = nullptr;

        size_t string_array_size = 0;

        void* entry;

        ELF(void* preferably_mmaped);
        ~ELF();

        bool isValid(bitness_t desired_bitness, endianiness_t desired_endianiness,
                     isa_t desired_isa);

        void loadSymbols();
        void loadStrings();

        void loadRelocations();

        // private:
        header   m_header;
        uint8_t* m_addr;

        void loadSymbols64();
        void loadSymbols32();

        void loadRelocations64();
        void loadRelocationsFromSection64(section_header_agn section);
    };
}