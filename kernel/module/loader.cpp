#include "aex/assert.hpp"
#include "aex/debug.hpp"
#include "aex/elf.hpp"
#include "aex/errno.hpp"
#include "aex/fs.hpp"
#include "aex/mem.hpp"
#include "aex/module.hpp"
#include "aex/printk.hpp"
#include "aex/proc.hpp"

#include "boot/mboot.h"
#include "elf.hpp"
#include "kernel/module.hpp"
#include "proc/proc.hpp"

namespace AEX {
    extern Mem::SmartArray<Module> modules;

    bool relocate(const char* path, ELF& elf, Module* module, module_section* sections);

    error_t load_module(const char* path, bool block) {
        auto file_try = FS::File::open(path);
        if (!file_try)
            return file_try.error_code;

        auto    file = file_try.value;
        int64_t size = file->seek(0, FS::File::SEEK_END).value;

        auto mmap_try = Mem::mmap(nullptr, size, Mem::PROT_READ, Mem::MAP_NONE, file, 0);
        file->close();

        if (!mmap_try)
            return mmap_try;

        void* addr = mmap_try.value;

        auto error = load_module(path, addr, size, block);
        Mem::munmap(addr, size);

        return error;
    }

    error_t load_module(const char* label, void* m_addr, size_t, bool block) {
        AEX_ASSERT(Debug::symbols_loaded);

        static Mutex lock = Mutex();
        ScopeMutex   scopeLock(lock);

        uint8_t* addr = (uint8_t*) m_addr;
        auto     elf  = ELF(addr);

        if (!elf.isValid(ELF::BIT_64, ELF::EN_LITTLE, ELF::ISA_AMD64))
            return ENOEXEC;

        elf.loadStrings();
        elf.loadSymbols();

        ELF::symbol_agn entry = {};
        ELF::symbol_agn exit  = {};
        ELF::symbol_agn name  = {};

        for (int i = 0; i < elf.symbols.count(); i++) {
            auto symbol = elf.symbols[i];
            if (!symbol.name)
                continue;

            if (strcmp(symbol.name, "_Z12module_enterv") == 0) {
                entry = elf.symbols[i];
                continue;
            }
            else if (strcmp(symbol.name, "_Z11module_exitv") == 0) {
                exit = elf.symbols[i];
                continue;
            }
            else if (strcmp(symbol.name, "MODULE_NAME") == 0) {
                name = elf.symbols[i];
                continue;
            }
        }

        if (entry.section_index == 0 || exit.section_index == 0 || name.section_index == 0)
            return ENOEXEC;

        auto sections = new module_section[elf.section_headers.count()];
        auto module   = new Module();

        for (int i = 0; i < elf.section_headers.count(); i++) {
            auto section_header = elf.section_headers[i];

            if (!(section_header.flags & ELF::SC_ALLOC) || section_header.size == 0)
                continue;

            void* ptr = Mem::kernel_pagemap->alloc(section_header.size, PAGE_WRITE | PAGE_EXEC);
            if (!(section_header.flags & ELF::SC_ALLOC))
                continue;

            // 6 hours of debugging for this god forsaken thing (the entire if)
            if (section_header.type != ELF::sc_type_t::SC_NO_DATA)
                memcpy(ptr, addr + section_header.file_offset, section_header.size);

            sections[i].addr = ptr;
            sections[i].size = section_header.size;

            module->sections.pushBack(sections[i]);
        }

        for (int i = 0; i < elf.symbols.count(); i++) {
            auto symbol = elf.symbols[i];
            if (!symbol.name)
                continue;

            if (symbol.name[0] == '.')
                continue;

            auto m_symbol = module_symbol();
            m_symbol.name = symbol.name;

            if (symbol.section_index == ELF::sym_special::SHN_ABS)
                m_symbol.addr = (void*) symbol.address;
            else if (symbol.section_index & 0xFF00)
                printk("module: Unknown section index encountered (0x%x)\n", symbol.section_index);
            else
                m_symbol.addr =
                    (void*) ((size_t) sections[symbol.section_index].addr + symbol.address);

            module->symbols.pushBack(m_symbol);
        }

        for (int i = 0; i < elf.symbols.count(); i++) {
            auto symbol = elf.symbols[i];
            if (!symbol.name)
                continue;

            if (symbol.name[0] != '.')
                continue;

            auto m_symbol = module_symbol();
            m_symbol.name = symbol.name;
            m_symbol.addr = (void*) ((size_t) sections[symbol.section_index].addr + symbol.address);

            module->symbols.pushBack(m_symbol);
        }

        elf.loadRelocations();
        bool success = relocate(label, elf, module, sections);

        if (!success) {
            delete module;
            delete[] sections;

            return ENOSYS;
        }

        module->name =
            *((const char**) ((size_t) sections[name.section_index].addr + name.address));
        module->name_len = strlen(module->name);

        module->enter = (void (*)())((size_t) sections[entry.section_index].addr + entry.address);
        module->exit  = (void (*)())((size_t) sections[exit.section_index].addr + exit.address);

        bool already_loaded = false;

        for (auto iterator = modules.getIterator(); auto m_module = iterator.next();) {
            if (strcmp(module->name, m_module->name) != 0)
                continue;

            already_loaded = true;
            break;
        }

        if (already_loaded) {
            printk(PRINTK_WARN "Not loading module '%s' as it's already loaded\n", module->name);

            delete module;
            delete[] sections;
            return ENONE;
        }

        int string_array_size = (module->name_len + 2) * module->symbols.count();

        for (int i = 0; i < module->symbols.count(); i++)
            string_array_size += strlen(module->symbols[i].name);

        module->strings = new char[string_array_size];

        int str_ptr = 0;

        for (int i = 0; i < module->symbols.count(); i++) {
            auto&       symbol = module->symbols[i];
            const char* ptr    = module->strings + str_ptr;

            int sym_name_len = strlen(symbol.name);

            memcpy(module->strings + str_ptr, module->name, module->name_len + 1);
            str_ptr += module->name_len;

            module->strings[str_ptr] = ':';
            str_ptr++;

            memcpy(module->strings + str_ptr, symbol.name, sym_name_len + 1);
            str_ptr += sym_name_len + 1;

            symbol.name = ptr;
        }

        modules.addRef(module);

        printk(PRINTK_OK "Loaded module '%s'\n", module->name);

        for (int i = 0; i < module->references.count(); i++)
            printk("module: %s references %s\n", module->name, module->references[i]->name);

        if (!Proc::ready) {
            ((void (*)()) module->enter)();

            delete[] sections;
            return ENONE;
        }

        // 2 goddamned hours + sleep for this goddamned thing (stack size)
        auto thread = new Proc::Thread(Proc::processes.get(1).get(), (void*) module->enter, 16384,
                                       Mem::kernel_pagemap);

        if (block) {
            Proc::Thread::getCurrent()->addCritical();

            thread->start();
            if (block)
                thread->join();

            Proc::Thread::getCurrent()->subCritical();
            Proc::Thread::yield();

            delete[] sections;
            return ENONE;
        }

        thread->start();

        delete[] sections;
        return ENONE;
    }
}