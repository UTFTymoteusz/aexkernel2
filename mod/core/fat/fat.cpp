#include "fat.hpp"

#include "aex/debug.hpp"
#include "aex/dev.hpp"
#include "aex/printk.hpp"

#include "controlblock.hpp"
#include "types.hpp"

// TOOD: Bound checks
namespace AEX::FS {
    FAT* fat_fs;

    void FAT::init() {
        fat_fs = new FAT();
    }

    FAT::FAT() : Filesystem("fat") {}
    FAT::~FAT() {}

    optional<ControlBlock*> FAT::mount(const char* source) {
        printk("fat: Mount attempt from %s\n", source);

        if (!source) {
            printk(PRINTK_WARN "fat: mount: Source not set\n");
            return EINVAL;
        }

        auto info = File::info(source);
        if (!info) {
            printk(PRINTK_WARN "fat: mount: Failed to get file info\n");
            return info.error_code;
        }

        if (!info.value.is_block()) {
            printk(PRINTK_WARN "fat: mount: Source is not a block device\n");
            return ENOTBLK;
        }

        auto handle_try = Dev::open_block_handle(info.value.dev);
        if (!handle_try) {
            printk(PRINTK_WARN "fat: mount: Failed to open the block device\n");
            return ENOENT;
        }

        auto handle   = handle_try.value;
        auto fat_info = gatherInfo(handle);

        if (fat_info.type == FAT_NONE)
            return EINVAL;

        if (fat_info.type != FAT_FAT32 || (fat_info.sector_size % 256) != 0)
            return EBOTHER;

        return new FATControlBlock(handle, fat_info);
    }

    // TODO: Improve this
    fat_info FAT::gatherInfo(Dev::BlockHandle handle) {
        fat_bpb  bpb;
        fat_info info;

        handle.read(&bpb, 0, sizeof(fat_bpb));

        uint32_t sector_count = bpb.total_sector_count ?: bpb.large_sector_count;

        info.type        = FAT_NONE;
        info.sector_size = bpb.bytes_per_sector;

        info.fat_start = bpb.reserved_sector_count * info.sector_size;
        info.fat_count = bpb.fat_count;

        info.cluster_size = bpb.bytes_per_sector * bpb.sectors_per_cluster;

        if (inrange(bpb.ebpb16.signature, 0x28, 0x29)) {
            info.type               = FAT_FAT16;
            info.fat_size           = bpb.fat_sector_count * info.sector_size;
            info.data_offset        = info.fat_start + info.fat_size * info.fat_count;
            info.cluster_count      = (sector_count - info.data_offset) / info.cluster_size;
            info.root_first_cluster = 0;
        }

        if (inrange(bpb.ebpb32.signature, 0x28, 0x29)) {
            info.type               = FAT_FAT32;
            info.fat_size           = bpb.ebpb32.fat_sector_count * info.sector_size;
            info.data_offset        = info.fat_start + info.fat_size * info.fat_count;
            info.cluster_count      = (sector_count - info.data_offset) / info.cluster_size;
            info.root_first_cluster = bpb.ebpb32.root_cluster - 2;
        }

        return info;
    }
}