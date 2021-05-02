#include "aex/dev.hpp"
#include "aex/kpanic.hpp"
#include "aex/mem.hpp"
#include "aex/spinlock.hpp"

namespace AEX::Dev {
    struct letter_incrementation {
        char pattern[32];
        char current[32];

        int start = 0;

        letter_incrementation(const char* pattern) {
            memset(this->current, '\0', sizeof(this->current));

            strncpy(this->pattern, pattern, sizeof(this->pattern));
            strncpy(this->current, pattern, sizeof(this->current));

            for (size_t i = 0; i < sizeof(this->current); i++) {
                if (this->current[i] == '\0')
                    kpanic("Invalid name pattern");

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

    struct number_incrementation {
        char pattern[32];
        char base[32];

        int current = 0;

        number_incrementation(const char* pattern) {
            strncpy(this->pattern, pattern, sizeof(this->pattern));
            strncpy(this->base, pattern, sizeof(this->base));

            for (size_t i = 0; i < sizeof(this->base); i++)
                if (this->base[i] == '%')
                    this->base[i] = '\0';
        }

        void get(char* buffer, size_t len) {
            snprintf(buffer, len, "%s%i", base, current);
            current++;
        }
    };

    Mem::Vector<letter_incrementation> letter_incrementations;
    Mem::Vector<number_incrementation> number_incrementations;

    Spinlock name_lock;

    void name_letter_increment(char* buffer, size_t buffer_len, const char* pattern) {
        SCOPE(name_lock);

        for (int i = 0; i < letter_incrementations.count(); i++) {
            auto incrementation = letter_incrementations[i];
            if (strcmp(incrementation.pattern, pattern) != 0)
                continue;

            letter_incrementations[i].get(buffer, buffer_len);
            return;
        }

        auto new_incrementation = letter_incrementation(pattern);

        int index = letter_incrementations.push(new_incrementation);
        letter_incrementations[index].get(buffer, buffer_len);
    }

    void name_number_increment(char* buffer, size_t buffer_len, const char* pattern) {
        SCOPE(name_lock);

        for (int i = 0; i < number_incrementations.count(); i++) {
            auto incrementation = number_incrementations[i];
            if (strcmp(incrementation.pattern, pattern) != 0)
                continue;

            number_incrementations[i].get(buffer, buffer_len);
            return;
        }

        auto new_incrementation = number_incrementation(pattern);

        int index = number_incrementations.push(new_incrementation);
        number_incrementations[index].get(buffer, buffer_len);
    }
}