#pragma once

#include "aex/errno.hpp"
#include "aex/mem/vector.hpp"
#include "aex/utility.hpp"

namespace AEX {
    struct API module_symbol {
        const char* name;
        void*       addr;
    };

    struct API module_section {
        void*  addr = nullptr;
        size_t size = 0;

        module_section() {}
    };

    class API Module {
        public:
        const char* name     = nullptr;
        size_t      name_len = 0;

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

            referencedBy.push(referencer);
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

            references.push(reference);
        }
    };

    API error_t load_module(const char* path);

    API void  register_dynamic_symbol(const char* name, void* addr);
    API void* get_dynamic_symbol(const char* name);
}