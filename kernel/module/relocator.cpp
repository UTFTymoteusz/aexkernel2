#include "aex/debug.hpp"
#include "aex/elf.hpp"
#include "aex/module.hpp"
#include "aex/printk.hpp"

#include "elf.hpp"
#include "kernel/module.hpp"

namespace AEX {
    bool relocate(const char* label, ELF& elf, Module* module, module_section* sections) {
        bool success = true;

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

                    S = (size_t) sections[i].addr;
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

                    S = (size_t) sections[symbol_b.section_index].addr + symbol_b.address;
                    break;
                }

            if (!S && (symbol.info >> 4) != ELF::sym_binding::SB_LOCAL)
                S = (uint64_t) Debug::name2addr(symbol.name);

            if (!S && (symbol.info >> 4) != ELF::sym_binding::SB_LOCAL) {
                Module* dawwdy;

                S = (uint64_t) module_name2addr_raw(symbol.name, dawwdy);
                if (S) {
                    dawwdy->addReferencedBy(module);
                    module->addReference(dawwdy);
                }
            }

            if (!S) {
                printk(PRINTK_WARN "module: %s: Unresolved symbol: %s\n", label, symbol.name);
                success = false;

                continue;
            }

            size_t self_addr =
                (size_t) sections[relocation.target_section_id].addr + relocation.addr;
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
                kpanic("module: Unknown relocation type encountered: %i",
                       relocation.arch_info & 0xFFFFFFFF);
                break;
            }
        }

        return success;
    }
}