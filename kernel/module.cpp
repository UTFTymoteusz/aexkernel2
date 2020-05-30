#include "aex/module.hpp"

#include "aex/debug.hpp"
#include "aex/elf.hpp"
#include "aex/errno.hpp"
#include "aex/fs/fs.hpp"
#include "aex/mem/vmem.hpp"
#include "aex/proc/proc.hpp"
#include "aex/proc/process.hpp"
#include "aex/proc/thread.hpp"

#include "elf.hpp"

namespace AEX {
    struct module_section {
        void*  addr = nullptr;
        size_t size = 0;

        module_section() {}
    };

    class Module {
      public:
        const char* name = nullptr;

        void (*enter)();
        void (*exit)();

        Mem::Vector<module_section> sections;

        ~Module() {
            for (int i = 0; i < sections.count(); i++)
                ; // implement paging dealloc pls
        }
    };

    error_t load_module(const char* path) {
        auto file_try = FS::File::open(path);
        if (!file_try.has_value)
            return file_try.error_code;

        auto file = file_try.value;
        auto elf  = ELF(file);

        if (!elf.isValid(ELF::bitness_t::BITS64, ELF::endianiness_t::LITTLE, ELF::isa_t::AMD64))
            return error_t::ENOEXEC;

        size_t symbol_count = elf.symbol_table.size / sizeof(ELF::symbol);
        auto   symbols      = new ELF::symbol[symbol_count];

        file->seek(elf.symbol_table.file_offset);
        file->read(symbols, symbol_count * sizeof(ELF::symbol));

        ELF::symbol entry_symbol;
        ELF::symbol exit_symbol;
        ELF::symbol name_symbol;

        for (size_t i = 0; i < symbol_count; i++) {
            const char* name = elf.strings + symbols[i].name_offset;

            if (strcmp(name, "_Z12module_enterv") == 0) {
                entry_symbol = symbols[i];
                continue;
            }
            else if (strcmp(name, "_Z11module_exitv") == 0) {
                exit_symbol = symbols[i];
                continue;
            }
            else if (strcmp(name, "MODULE_NAME") == 0) {
                name_symbol = symbols[i];
                continue;
            }
        }

        if (entry_symbol.symbol_index == 0 || exit_symbol.symbol_index == 0 ||
            name_symbol.symbol_index == 0) {
            delete symbols;

            return error_t::ENOEXEC;
        }

        auto section_info = new module_section[elf.section_headers.count()];
        auto module       = new Module();

        for (int i = 0; i < elf.section_headers.count(); i++) {
            auto section_header = elf.section_headers[i];

            if (!(section_header.flags & ELF::sc_flags_t::SC_ALLOC))
                continue;

            file->seek(section_header.file_offset);

            void* ptr = VMem::kernel_pagemap->alloc(section_header.size);

            file->read(ptr, section_header.size);

            section_info[i].addr = ptr;
            section_info[i].size = section_header.size;

            module->sections.pushBack(section_info[i]);
            // printk("%s [%i] is full'o'program data (0x%p)\n", section_header.name, i, ptr);
        }

        bool fail = false;

        for (int i = 0; i < elf.section_headers.count(); i++) {
            auto section_header = elf.section_headers[i];

            if (!(section_header.type & ELF::sc_type_t::SC_RELOCA))
                continue;

            // printk("%s is full'o'relocations (relates to %i)\n", section_header.name,
            //       section_header.info);
            file->seek(section_header.file_offset);

            for (size_t j = 0; j < section_header.size / sizeof(ELF::relocation_addend); j++) {
                ELF::relocation_addend relocation;
                file->read(&relocation, sizeof(relocation));

                uint32_t symbol_id = (relocation.info >> 32);
                auto     symbol    = symbols[symbol_id];

                const char* name = elf.strings + symbol.name_offset;
                if (name[0] == '\0')
                    name = elf.section_headers[symbol.symbol_index].name;

                size_t dst_addr = 0;
                if (!dst_addr)
                    dst_addr = (uint64_t) Debug::symbol_name2addr(name);

                if (!dst_addr)
                    for (size_t i = 0; i < symbol_count; i++) {
                        if (strcmp(elf.strings + symbols[i].name_offset, name) != 0)
                            continue;

                        dst_addr =
                            (size_t) section_info[symbol.symbol_index].addr + symbols[i].address;
                        break;
                    }

                if (!dst_addr)
                    for (int i = 0; i < elf.section_headers.count(); i++) {
                        auto section_header = elf.section_headers[i];
                        if (strcmp(section_header.name, name) != 0)
                            continue;

                        dst_addr = (size_t) section_info[i].addr;
                        break;
                    }

                if (!dst_addr) {
                    printk(PRINTK_WARN "module: Unresolved symbol: %s\n", name);
                    fail = true;
                }

                size_t self_addr =
                    (size_t) section_info[section_header.info].addr + relocation.addr;
                void* dst = (void*) self_addr;

                int64_t addend = relocation.addend;

                switch ((amd64_rel_type) relocation.info & 0xFFFFFFFF) {
                case R_AMD64_64:
                    *((uint64_t*) dst) = dst_addr + addend;
                    break;
                case R_AMD64_PC32:
                    *((int32_t*) dst) =
                        (int32_t)((int64_t) dst_addr - (int64_t) self_addr + addend);
                    break;
                case R_AMD64_PLT32:
                    *((int32_t*) dst) =
                        (int32_t)((int64_t) dst_addr - (int64_t) self_addr + addend);
                    break;
                case R_AMD64_32S:
                    *((int32_t*) dst) = dst_addr + addend;
                    break;
                default:
                    kpanic("module: Unknown relocation type encountered: %i\n",
                           relocation.info & 0xFFFFFFFF);
                    break;
                }

                // printk("0x%p v. %i: [%i] %s + %li (to 0x%p)\n", dst, symbol.symbol_index,
                // symbol_id,
                //       name, relocation.addend, dst_addr);
            }
        }

        if (fail) {
            delete module;

            delete symbols;
            delete section_info;

            return error_t::ENOSYS;
        }

        module->name =
            *((const char**) section_info[name_symbol.symbol_index].addr + name_symbol.address);

        module->enter = (void (*)())((size_t) section_info[entry_symbol.symbol_index].addr +
                                     entry_symbol.address);
        module->exit  = (void (*)())((size_t) section_info[exit_symbol.symbol_index].addr +
                                    exit_symbol.address);

        printk(PRINTK_OK "Loaded module '%s'\n", module->name);

        auto thread =
            new Proc::Thread(Proc::processes.get(1).get(), (void*) module->enter,
                             VMem::kernel_pagemap->alloc(8192), 8192, VMem::kernel_pagemap);
        thread->start();

        delete symbols;
        delete section_info;

        return error_t::ENONE;
    }
}