#pragma once

#include "aex/errno.hpp"
#include "aex/mem/vector.hpp"

namespace AEX {
    struct module_symbol {
        const char* name;
        void*       addr;
    };

    struct module_section {
        void*  addr = nullptr;
        size_t size = 0;

        module_section() {}
    };

    class Module {
        public:
        const char* name     = nullptr;
        int         name_len = 0;

        char* strings = nullptr;

        void (*enter)();
        void (*exit)();

        Mem::Vector<module_section> sections;
        Mem::Vector<module_symbol>  symbols;

        Mem::Vector<Module*> references;
        Mem::Vector<Module*> referencedBy;

        ~Module() {
            if (strings)
                delete[] strings;

            for (int i = 0; i < references.count(); i++)
                references[i]->removeReferencedBy(this);

            for (int i = 0; i < sections.count(); i++)
                ; // implement paging dealloc pls
        }

        void addReferencedBy(Module* referencer) {
            for (int i = 0; i < referencedBy.count(); i++)
                if (referencedBy[i] == referencer)
                    return;

            referencedBy.pushBack(referencer);
        }

        void removeReferencedBy(Module* referencer) {
            for (int i = 0; i < referencedBy.count(); i++) {
                if (referencedBy[i] != referencer)
                    return;

                referencedBy.erase(i);
                i--;

                break;
            }
        }

        void addReference(Module* reference) {
            for (int i = 0; i < references.count(); i++)
                if (references[i] == reference)
                    return;

            references.pushBack(reference);
        }
    };

    error_t load_module(const char* path);

    void  register_dynamic_symbol(const char* name, void* addr);
    void* get_dynamic_symbol(const char* name);
}