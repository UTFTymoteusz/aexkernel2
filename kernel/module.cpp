#include "aex/module.hpp"

#include "aex/debug.hpp"
#include "aex/elf.hpp"
#include "aex/errno.hpp"
#include "aex/fs.hpp"
#include "aex/mem.hpp"
#include "aex/printk.hpp"
#include "aex/proc.hpp"

#include "boot/mboot.h"
#include "elf.hpp"
#include "kernel/module.hpp"
#include "proc/proc.hpp"

namespace AEX {
    struct module_symbol {
        char  name[64];
        void* addr;
    };

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
        Mem::Vector<module_symbol>  symbols;

        ~Module() {
            for (int i = 0; i < sections.count(); i++)
                ; // implement paging dealloc pls
        }
    };

    Mem::SmartArray<Module>    modules;
    Mem::Vector<module_symbol> global_symbols;

    Spinlock symbol_lock;

    error_t load_module(const char* path, bool block) {
        auto file_try = FS::File::open(path);
        if (!file_try.has_value)
            return file_try.error_code;

        auto    file = file_try.value;
        int64_t size = file->seek(0, FS::File::END).value;

        auto mmap_try = Mem::mmap(nullptr, size, Mem::PROT_READ, Mem::MAP_NONE, file, 0);
        if (!mmap_try.has_value)
            return mmap_try.error_code;

        file->close();

        void* addr = mmap_try.value;

        auto error = load_module_from_memory(addr, size, path, block);
        Mem::munmap(addr, size);

        return error;
    }

    error_t load_module_from_memory(void* _addr, size_t, const char* path, bool block) {
        static Mutex lock      = Mutex();
        auto         scopeLock = ScopeMutex(lock);

        uint8_t* addr = (uint8_t*) _addr;
        auto     elf  = ELF(addr);

        if (!elf.isValid(ELF::bitness_t::BITS64, ELF::endianiness_t::LITTLE, ELF::isa_t::AMD64))
            return ENOEXEC;

        elf.loadStrings();
        elf.loadSymbols();

        ELF::symbol_agnostic entry_symbol = {};
        ELF::symbol_agnostic exit_symbol  = {};
        ELF::symbol_agnostic name_symbol  = {};

        for (int i = 0; i < elf.symbols.count(); i++) {
            auto symbol = elf.symbols[i];
            if (!symbol.name)
                continue;

            if (strcmp(symbol.name, "_Z12module_enterv") == 0) {
                entry_symbol = elf.symbols[i];
                continue;
            }
            else if (strcmp(symbol.name, "_Z11module_exitv") == 0) {
                exit_symbol = elf.symbols[i];
                continue;
            }
            else if (strcmp(symbol.name, "MODULE_NAME") == 0) {
                name_symbol = elf.symbols[i];
                continue;
            }
        }

        if (entry_symbol.section_index == 0 || exit_symbol.section_index == 0 ||
            name_symbol.section_index == 0)
            return ENOEXEC;

        auto section_info = new module_section[elf.section_headers.count()];
        auto module       = new Module();

        for (int i = 0; i < elf.section_headers.count(); i++) {
            auto section_header = elf.section_headers[i];

            if (!(section_header.flags & ELF::sc_flags_t::SC_ALLOC))
                continue;

            void* ptr = Mem::kernel_pagemap->alloc(section_header.size, PAGE_WRITE | PAGE_EXEC);
            if (!(section_header.flags & ELF::sc_flags_t::SC_ALLOC))
                continue;

            // 6 hours of debugging for this god forsaken thing (the entire if)
            if (section_header.type != ELF::sc_type_t::SC_NO_DATA)
                memcpy(ptr, addr + section_header.file_offset, section_header.size);

            section_info[i].addr = ptr;
            section_info[i].size = section_header.size;

            module->sections.pushBack(section_info[i]);
        }

        for (int i = 0; i < elf.symbols.count(); i++) {
            auto symbol = elf.symbols[i];
            if (!symbol.name)
                continue;

            if (symbol.name[0] == '.')
                continue;

            auto _symbol = module_symbol();

            strncpy(_symbol.name, symbol.name, sizeof(_symbol.name));

            if (symbol.section_index == ELF::sym_special::SHN_ABS)
                _symbol.addr = (void*) symbol.address;
            else if (symbol.section_index & 0xFF00)
                printk("module: Unknown section index encountered (0x%x)\n", symbol.section_index);
            else
                _symbol.addr =
                    (void*) ((size_t) section_info[symbol.section_index].addr + symbol.address);

            module->symbols.pushBack(_symbol);
        }

        for (int i = 0; i < elf.symbols.count(); i++) {
            auto symbol = elf.symbols[i];
            if (!symbol.name)
                continue;

            if (symbol.name[0] != '.')
                continue;

            auto _symbol = module_symbol();

            strncpy(_symbol.name, symbol.name, sizeof(_symbol.name));
            _symbol.addr =
                (void*) ((size_t) section_info[symbol.section_index].addr + symbol.address);

            module->symbols.pushBack(_symbol);
        }

        elf.loadRelocations();

        bool fail = false;

        for (int i = 0; i < elf.relocations.count(); i++) {
            auto relocation = elf.relocations[i];

            auto symbol = elf.symbols[relocation.symbol_id];
            if (!symbol.name)
                continue;

            size_t S = 0;

            // Gotta make weaklings work properly
            // if ((symbol.info >> 4) == 2)
            //    printk(PRINTK_WARN "module: %s: Weak\n", symbol.name);

            if (!S)
                for (int i = 0; i < elf.section_headers.count(); i++) {
                    auto section_header = elf.section_headers[i];
                    if (!section_header.name)
                        continue;

                    if (strcmp(symbol.name, section_header.name) != 0)
                        continue;

                    S = (size_t) section_info[i].addr;
                    break;
                }

            if (!S)
                for (int j = 0; j < elf.symbols.count(); j++) {
                    auto symbol_b = elf.symbols[j];
                    if (!symbol_b.name)
                        continue;

                    if (symbol_b.section_index == 0)
                        continue;

                    if (strcmp(symbol.name, symbol_b.name) != 0)
                        continue;

                    S = (size_t) section_info[symbol_b.section_index].addr + symbol_b.address;
                    break;
                }

            if (!S && (symbol.info >> 4) != ELF::sym_binding::SB_LOCAL)
                S = (uint64_t) Debug::symbol_name2addr(symbol.name);

            if (!S) {
                printk(PRINTK_WARN "module: %s: Unresolved symbol: %s\n", path, symbol.name);
                fail = true;

                continue;
            }

            size_t self_addr =
                (size_t) section_info[relocation.target_section_id].addr + relocation.addr;
            void* self = (void*) self_addr;

            size_t  P = self_addr;
            int64_t A = relocation.addend;

            switch ((amd64_rel_type) relocation.arch_info & 0xFFFFFFFF) {
            case R_AMD64_64:
                *((uint64_t*) self) = S + A;
                break;
            case R_AMD64_PC32:
                *((int32_t*) self) = (int32_t)(S - P + A);
                break;
            case R_AMD64_PLT32:
                *((int32_t*) self) = (int32_t)(S - P + A);
                break;
            case R_AMD64_32S:
                *((int32_t*) self) = S + A;
                break;
            default:
                kpanic("module: Unknown relocation type encountered: %i\n",
                       relocation.arch_info & 0xFFFFFFFF);
                break;
            }
        }

        if (fail) {
            delete module;
            delete section_info;

            return ENOSYS;
        }

        module->name = *((const char**) ((size_t) section_info[name_symbol.section_index].addr +
                                         name_symbol.address));

        module->enter = (void (*)())((size_t) section_info[entry_symbol.section_index].addr +
                                     entry_symbol.address);
        module->exit  = (void (*)())((size_t) section_info[exit_symbol.section_index].addr +
                                    exit_symbol.address);

        bool already_loaded = false;

        for (auto iterator = modules.getIterator(); auto _module = iterator.next();) {
            if (strcmp(module->name, _module->name) != 0)
                continue;

            already_loaded = true;
            break;
        }

        if (already_loaded) {
            printk(PRINTK_WARN "Not loading module '%s' as it's already loaded\n", module->name);

            delete module;
            delete section_info;
            return ENONE;
        }

        modules.addRef(module);

        for (int i = 0; i < module->symbols.count(); i++) {
            char buffer[sizeof(module->symbols[i].name)];

            strncpy(buffer, module->symbols[i].name, sizeof(buffer));
            snprintf(module->symbols[i].name, sizeof(module->symbols[i].name), "%s%c%s",
                     module->name, ':', buffer);
        }

        printk(PRINTK_OK "Loaded module '%s'\n", module->name);

        if (!Proc::ready) {
            ((void (*)()) module->enter)();

            delete section_info;
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

            delete section_info;
            return ENONE;
        }

        thread->start();

        delete section_info;
        return ENONE;
    }

    void load_multiboot_symbols(multiboot_info_t* mbinfo) {
        if (!(mbinfo->flags & MULTIBOOT_INFO_MODS))
            return;

        auto list = (multiboot_mod_list*) (size_t) mbinfo->mods_addr;

        for (uint32_t i = 0; i < mbinfo->mods_count; i++) {
            if (list[i].cmdline == 0 || strcmp((char*) (size_t) list[i].cmdline, "kernel") != 0)
                continue;

            Debug::load_kernel_symbols_from_memory((void*) (size_t) list[i].mod_start);
        }
    }

    void load_multiboot_modules(multiboot_info_t* mbinfo) {
        if (!(mbinfo->flags & MULTIBOOT_INFO_MODS))
            return;

        auto list = (multiboot_mod_list*) (size_t) mbinfo->mods_addr;

        if (!Debug::symbols_loaded)
            return;

        for (uint32_t i = 0; i < mbinfo->mods_count; i++) {
            if (list[i].cmdline != 0 && strcmp((char*) (size_t) list[i].cmdline, "kernel") == 0)
                continue;

            load_module_from_memory((void*) (size_t) list[i].mod_start,
                                    list[i].mod_end - list[i].mod_start,
                                    (char*) (size_t) list[i].cmdline, true);
        }
    }

    // This will need to be changed in the future, though
    // Or I'll just need to make the /sys/core/ directory not modifyable by users
    void load_core_modules() {
        struct module_entry {
            char name[FS::Path::MAX_FILENAME_LEN];
            int  order = 99999;
        };

        auto dir_try = FS::File::opendir("/sys/core/");
        if (!dir_try.has_value) {
            printk(PRINTK_WARN "module: Failed to opendir /sys/core/: %s\n",
                   strerror(dir_try.error_code));

            return;
        }

        auto get_order = [](FS::dir_entry& dir_entry) {
            auto name = (char*) &dir_entry.name;

            while (*name && (*name != '.'))
                name++;

            if (*name == '\0')
                return 999999;

            name++;

            char parse_buffer[16] = {};
            int  pos              = 0;

            while (*name && (*name != '.') && pos != 15) {
                parse_buffer[pos] = *name;

                pos++;
                name++;
            }

            return stoi<int>(10, parse_buffer);
        };

        auto dir  = dir_try.value;
        auto list = Mem::Vector<module_entry>();

        while (true) {
            auto entry_try = dir->readdir();
            if (!entry_try.has_value)
                break;

            if (!entry_try.value.is_regular())
                continue;

            auto entry = module_entry();

            strncpy(entry.name, entry_try.value.name, sizeof(entry.name));
            entry.order = get_order(entry_try.value);

            list.pushBack(entry);
        }

        // No need for a fancy algorithm atm
        for (int i = 0; i < list.count() - 1; i++) {
            for (int j = 0; j < list.count() - 1; j++) {
                if (list[j].order > list[j + 1].order) {
                    auto tmp    = list[j];
                    list[j]     = list[j + 1];
                    list[j + 1] = tmp;
                }
            }
        }

        for (int i = 0; i < list.count(); i++) {
            char buffer[FS::Path::MAX_PATH_LEN];

            FS::Path::canonize_path(list[i].name, "/sys/core/", buffer, sizeof(buffer));

            load_module(buffer, true);

            Proc::Thread::sleep(75);
        }

        dir->close();
    }

    const char* module_symbol_addr2name(void* addr, int* delta_ret) {
        uint64_t _addr = (uint64_t) addr;
        uint64_t delta = 0x63756e74;

        module_symbol match = {};
        const char*   name  = nullptr;

        for (auto iterator = modules.getIterator(); auto module = iterator.next();)
            for (int i = 0; i < module->symbols.count(); i++) {
                auto symbol = module->symbols[i];

                if (_addr < (size_t) symbol.addr)
                    continue;

                uint64_t new_delta = _addr - (size_t) symbol.addr;
                if (new_delta >= delta)
                    continue;

                delta = new_delta;
                match = symbol;

                name = module->symbols[i].name;
            }

        *delta_ret = delta;

        return name;
    }

    void register_global_symbol(const char* name, void* addr) {
        auto scopeLock = ScopeSpinlock(symbol_lock);

        if ((size_t) strlen(name) > sizeof(module_symbol::name) - 1)
            kpanic("register_global_symbol: name length > 31\n");

        for (int i = 0; i < global_symbols.count(); i++) {
            if (strcmp(name, global_symbols[i].name) == 0) {
                global_symbols[i].addr = addr;
                return;
            }
        }

        auto symbol = module_symbol();

        strncpy(symbol.name, name, sizeof(symbol.name));
        symbol.addr = addr;

        global_symbols.pushBack(symbol);
    }

    void* get_global_symbol(const char* name) {
        auto scopeLock = ScopeSpinlock(symbol_lock);

        for (int i = 0; i < global_symbols.count(); i++) {
            if (strcmp(name, global_symbols[i].name) == 0)
                return global_symbols[i].addr;
        }

        return nullptr;
    }
}