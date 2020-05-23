#include "aex/dev/name.hpp"

#include "aex/kpanic.hpp"
#include "aex/mem/vector.hpp"
#include "aex/spinlock.hpp"

#include <stddef.h>

namespace AEX::Dev {
    struct incrementation {
        char pattern[32];
        char current[32];

        int start = 0;

        incrementation(const char* pattern) {
            memset(this->current, '\0', sizeof(this->current));

            strncpy(this->pattern, pattern, sizeof(this->pattern));
            strncpy(this->current, pattern, sizeof(this->current));

            for (size_t i = 0; i < sizeof(this->current); i++) {
                if (this->current[i] == '\0')
                    kpanic("Invalid name pattern\n");

                if (this->current[i] == '%') {
                    start            = i;
                    this->current[i] = 'a';

                    break;
                }
            }
        }

        void get(char* buffer, size_t len) {
            strncpy(buffer, this->current, len);
            buffer[len - 1] = '\0';

            for (size_t i = start; i < sizeof(this->current); i++) {
                if (this->current[i] == '\0') {
                    this->current[i] = 'a';
                    break;
                }

                this->current[i]++;

                if (this->current[i] > 'z') {
                    this->current[i] = 'a';
                    continue;
                }

                break;
            }
        }
    };
    Mem::Vector<incrementation> incrementations;

    Spinlock name_lock;

    void name_letter_increment(char* buffer, size_t buffer_len, const char* pattern) {
        auto scopeLock = ScopeSpinlock(name_lock);

        for (int i = 0; i < incrementations.count(); i++) {
            auto incrementation = incrementations[i];
            if (strcmp(incrementation.pattern, pattern) != 0)
                continue;

            incrementations[i].get(buffer, buffer_len);

            return;
        }

        auto new_incrementation = incrementation(pattern);

        int index = incrementations.pushBack(new_incrementation);

        incrementations[index].get(buffer, buffer_len);
    }
}