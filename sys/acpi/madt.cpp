#include "aex/sys/acpi/madt.hpp"

#include <stddef.h>

namespace AEX::Sys::ACPI {
    void* madt::findEntry(int type, int index) {
        for (size_t i = 0; i < header.length - sizeof(madt);) {
            auto entry = (madt::entry*) &(data[i]);

            if (entry->type != type) {
                i += entry->len;
                continue;
            }

            if (index > 0) {
                index--;
                i += entry->len;

                continue;
            }

            return (void*) entry;
        }

        return nullptr;
    }
}