#include "aex/fs/part.hpp"

#include "aex/dev/blockdevice.hpp"
#include "aex/dev/blockhandle.hpp"
#include "aex/errno.hpp"
#include "aex/optional.hpp"
#include "aex/printk.hpp"
#include "aex/utility.hpp"

#include "fs/part.hpp"
#include "fs/part/msdos.hpp"
#include "fs/part/partition.hpp"

using namespace AEX::Dev;

namespace AEX::FS {
    error_t read_partitions(Dev::BlockHandle handle, int limit) {
        partition     partitions[limit];
        optional<int> part_try;

        if ((part_try = read_msdos(handle, partitions, limit))) {
            int count = part_try.value;

            for (int i = 0; i < count; i++) {
                char name[32];
                snprintf(name, sizeof(name), "%s%i", handle.getDev()->name, i + 1);

                auto partition = new Partition(name, handle.getDev().get(), partitions[i].start,
                                               partitions[i].count);
                if (!partition->registerDevice()) {
                    printk("part: %s: Failed to register device\n", name);
                    delete partition;
                }
            }
        }
        else {
            return ENOTDIR;
        }

        return ENONE;
    }
}