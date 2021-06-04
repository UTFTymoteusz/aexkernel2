#include "aex/printk.hpp"

#include "fat.hpp"

using namespace AEX;

const char* MODULE_NAME = "fat";

void module_enter() {
    FS::FAT::init();
}

void module_exit() {}