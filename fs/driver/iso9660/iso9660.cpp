#include "iso9660.hpp"

#include "aex/dev/block.hpp"
#include "aex/dev/dev.hpp"
#include "aex/fs/file.hpp"
#include "aex/printk.hpp"

#define BLOCK_SIZE 2048

namespace AEX::FS {
    void ISO9660::init() {
        new ISO9660();
    }

    ISO9660::ISO9660() : Filesystem("iso9660") {}
    ISO9660::~ISO9660() {}

    optional<Mount*> ISO9660::mount(const char* source) {
        printk("iso9660: Mount attempt from %s\n", source);

        if (!source)
            return error_t::EINVAL;

        auto info = File::info(source);
        if (!info.has_value)
            return info.error_code;

        if (!info.value.is_block())
            return error_t::ENOTBLK;

        auto block = (Mem::SmartPointer<Dev::Block>) Dev::devices.get(info.value.dev_id);
        if (!block.isValid())
            return error_t::ENOENT;

        char buffer[12];

        block->read((uint8_t*) &buffer, 0x38, 10);
        printk("read: %s\n", buffer);

        return new ISO9660Mount();
    }
}