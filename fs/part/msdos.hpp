#pragma once

#include "aex/dev/blockdevice.hpp"
#include "aex/dev/blockhandle.hpp"
#include "aex/types.hpp"
#include "aex/utility.hpp"

#include "fs/part.hpp"

namespace AEX::FS {
    struct msdos_partition {
        uint8_t  attributes;
        uint8_t  chs_start[3];
        uint8_t  type;
        uint8_t  chs_end[3];
        uint32_t lba_start;
        uint32_t lba_count;
    } PACKED;

    struct msdos_mbr {
        uint8_t         code[440];
        uint32_t        uuid;
        uint16_t        reserved;
        msdos_partition partitions[4];
        uint16_t        signature;
    } PACKED;

    optional<int> read_msdos(Dev::BlockHandle handle, partition* partitions, int limit);
}