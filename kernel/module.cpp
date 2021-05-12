#include "aex/module.hpp"

#include "aex/assert.hpp"
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
    Mem::SmartArray<Module>    modules;
    Mem::Vector<module_symbol> global_symbols;

    Spinlock symbol_lock;

    void load_symbols(multiboot_info_t* mbinfo) {
        auto list = (multiboot_mod_list*) (size_t) mbinfo->mods_addr;

        for (uint32_t i = 0; i < mbinfo->mods_count; i++) {
            if (list[i].cmdline == 0 ||
                strcmp((char*) (size_t) list[i].cmdline, "kernel_symbols") != 0)
                continue;

            Debug::load_symbols((void*) (size_t) list[i].mod_start);
        }
    }

    void load_modules(multiboot_info_t* mbinfo) {
        auto list = (multiboot_mod_list*) (size_t) mbinfo->mods_addr;

        for (uint32_t i = 0; i < mbinfo->mods_count; i++) {
            if (list[i].cmdline != 0 &&
                strcmp((char*) (size_t) list[i].cmdline, "kernel_symbols") == 0)
                continue;

            AEX_ASSERT(load_module((char*) (size_t) list[i].cmdline,
                                   (void*) (size_t) list[i].mod_start,
                                   list[i].mod_end - list[i].mod_start, true) == ENONE);
        }
    }

    // This will need to be changed in the future, though
    // Or I'll just need to make the /sys/mod/core/ directory not modifyable by users
    void load_core_modules() {
        struct module_entry {
            char name[FS::MAX_FILENAME_LEN];
            int  order = 99999;
        };

        auto dir_try = FS::File::open("/sys/mod/core/", FS::O_RDONLY);
        if (!dir_try) {
            printk(WARN "module: Failed to open /sys/mod/core/: %s\n", strerror(dir_try));
            return;
        }

        auto get_order = [](FS::dirent& dirent) {
            auto name = (char*) &dirent.name;

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
            if (!entry_try)
                break;

            if (!entry_try.value.is_regular())
                continue;

            auto entry = module_entry();

            strncpy(entry.name, entry_try.value.name, sizeof(entry.name));
            entry.order = get_order(entry_try.value);

            list.push(entry);
        }

        // No need for a fancy algorithm atm
        for (int i = 0; i < list.count() - 1; i++) {
            for (int j = 0; j < list.count() - 1; j++) {
                if (list[j].order <= list[j + 1].order)
                    continue;

                swap(list[j], list[j + 1]);
            }
        }

        for (int i = 0; i < list.count(); i++) {
            char name[FS::MAX_PATH_LEN];

            FS::canonize_path(list[i].name, "/sys/mod/core/", name, sizeof(name));
            load_module(name, true);
        }

        dir->close();
    }

    const char* module_addr2name(void* addr, int& delta_ret) {
        Module* mod;
        return module_addr2name(addr, delta_ret, mod);
    }

    const char* module_addr2name(void* addr, int& delta_ret, Module*& module_ref) {
        uint64_t m_addr = (uint64_t) addr;
        uint64_t delta  = 0x63756e74;

        module_symbol match = {};
        const char*   name  = nullptr;

        for (auto iterator = modules.getIterator(); auto module = iterator.next();)
            for (int i = 0; i < module->symbols.count(); i++) {
                auto symbol = module->symbols[i];

                if (m_addr < (size_t) symbol.addr)
                    continue;

                uint64_t new_delta = m_addr - (size_t) symbol.addr;
                if (new_delta >= delta || new_delta >= 0x2000)
                    continue;

                delta = new_delta;
                match = symbol;

                name       = module->symbols[i].name;
                module_ref = module;
            }

        delta_ret = delta;

        return name;
    }

    void* module_name2addr_raw(const char* name, Module*& module) {
        for (auto iterator = modules.getIterator(); (module = iterator.next());)
            for (int i = 0; i < module->symbols.count(); i++) {
                auto symbol = module->symbols[i];

                const char* clean_name = symbol.name + 1 + module->name_len;
                if (strcmp(clean_name, name) != 0)
                    continue;

                return symbol.addr;
            }

        module = nullptr;
        return nullptr;
    }

    void register_dynamic_symbol(const char* name, void* addr) {
        SCOPE(symbol_lock);

        for (int i = 0; i < global_symbols.count(); i++) {
            if (strcmp(name, global_symbols[i].name) == 0) {
                global_symbols[i].addr = addr;
                return;
            }
        }

        global_symbols.push({
            .name = strpivot(name, strlen(name)),
            .addr = addr,
        });
    }

    void* get_dynamic_symbol(const char* name) {
        SCOPE(symbol_lock);

        for (int i = 0; i < global_symbols.count(); i++) {
            if (strcmp(name, global_symbols[i].name) == 0)
                return global_symbols[i].addr;
        }

        return nullptr;
    }
}