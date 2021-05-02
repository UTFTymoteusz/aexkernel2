#include "msdos.hpp"

#include "aex/dev/blockdevice.hpp"
#include "aex/dev/blockhandle.hpp"

namespace AEX::FS {
    optional<int> read_msdos(Dev::BlockHandle handle, partition* partitions, int limit) {
        msdos_mbr mbr;
        handle.read(&mbr, 0, sizeof(mbr));

        if (mbr.signature != 0xAA55)
            return EINVAL;

        int part = 0;

        for (int i = 0; i < 4; i++) {
            auto partition = mbr.partitions[i];
            if (!partition.lba_count)
                continue;

            if (part == limit)
                break;

            partitions[part].start = partition.lba_start;
            partitions[part].count = partition.lba_count;

            part++;
        }

        return part;
    }
}