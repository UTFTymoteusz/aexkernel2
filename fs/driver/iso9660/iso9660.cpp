#include "iso9660.hpp"

#include "aex/dev/block.hpp"
#include "aex/printk.hpp"

namespace AEX::FS {
    void ISO9660::init() {
        new ISO9660();
    }

    ISO9660::ISO9660() : Filesystem("iso9660") {}
    ISO9660::~ISO9660() {}

    optional<Mount*> ISO9660::mount(const char* source) {
        printk("iso9660: Mount attempt\n");

        if (!source)
            return {};

        return {};
    }
}