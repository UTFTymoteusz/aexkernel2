#include "aex/sys/power.hpp"

#include "aex/arch/sys/cpu.hpp"
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
        SCOPE(action_mutex);

        printk(WARN "sys: power: Poweroff\n");

        for (auto& handler : poweroff_handlers) {
            if (handler.func() == ESHUTDOWN)
                break;
        }

        while (true)
            Sys::CPU::wait();

        return ESHUTDOWN;
    }

    void register_poweroff_handler(int order, error_t (*func)()) {
        SCOPE(action_mutex);

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
