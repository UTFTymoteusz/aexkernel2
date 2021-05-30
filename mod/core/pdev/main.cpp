#include "dev/full.hpp"
#include "dev/null.hpp"
#include "dev/random.hpp"
#include "dev/zero.hpp"

const char* MODULE_NAME = "pdev";

void module_enter() {
    auto null = new Null();
    if (!null->registerDevice()) {
        printk(WARN "pdev: Failed to register /dev/null\n");
        delete null;
    }

    auto zero = new Zero();
    if (!zero->registerDevice()) {
        printk(WARN "pdev: Failed to register /dev/zero\n");
        delete zero;
    }

    auto full = new Full();
    if (!full->registerDevice()) {
        printk(WARN "pdev: Failed to register /dev/full\n");
        delete full;
    }

    auto random = new Random();
    if (!random->registerDevice()) {
        printk(WARN "pdev: Failed to register /dev/random\n");
        delete random;
    }
}

void module_exit() {}