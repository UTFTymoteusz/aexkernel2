#include "aex/sys/power.hpp"

#include "aex/mem/vector.hpp"
#include "aex/mutex.hpp"
#include "aex/printk.hpp"
#include "aex/utility.hpp"

namespace AEX::Sys::Power {
    struct handler {
        int order;
        error_t (*func)();
    };

    Mutex                action_mutex;
    Mem::Vector<handler> poweroff_handlers;

    void sort(Mem::Vector<handler>& handlers);

    error_t poweroff() {
        auto scope = action_mutex.scope();

        printk(PRINTK_WARN "sys: power: Poweroff\n");

        for (int i = 0; i < poweroff_handlers.count(); i++)
            poweroff_handlers[i].func();

        return ENONE;
    }

    void register_poweroff_handler(int order, error_t (*func)()) {
        auto scope = action_mutex.scope();

        poweroff_handlers.push(handler{.order = order, .func = func});
        sort(poweroff_handlers);
    }

    void sort(Mem::Vector<handler>& handlers) {
        for (int i = 0; i < handlers.count() - 1; i++) {
            for (int j = 0; j < handlers.count() - 1; j++) {
                if (handlers[j].order <= handlers[j + 1].order)
                    continue;

                swap(handlers[i], handlers[i + 1]);
            }
        }
    }
}
