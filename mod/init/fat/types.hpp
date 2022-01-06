#pragma once

#include "aex/byte.hpp"
#include "aex/fs/types.hpp"
#include "aex/types.hpp"
#include "aex/utility.hpp"

namespace AEX::FS {
    typedef uint32_t cluster_t;

    enum fat_type {
        FAT_FAT12 = 0,
        FAT_FAT16 = 1,
        FAT_FAT32 = 2,
        FAT_NONE  = 255,
    };

    enum fat_attributes {
        FAT_READONLY  = 0x01,
        FAT_HIDDEN    = 0x02,
        FAT_SYSTEM    = 0x04,
        FAT_VOLUME_ID = 0x08,
        FAT_DIRECTORY = 0x10,
        FAT_ARCHIVE   = 0x20,
        FAT_LFN       = FAT_READONLY | FAT_HIDDEN | FAT_SYSTEM | FAT_VOLUME_ID,
    };

    struct fat_info {
        fat_type  type;
        blksize_t sector_size;

        off_t    fat_start;
        size_t   fat_size;
        uint16_t fat_count;

        off_t     data_offset;
        blksize_t cluster_size;
        uint32_t  cluster_count;

        cluster_t root_first_cluster;
    };

    struct fat_ebpb16 {
        uint8_t      drive_number;
        uint8_t      nt_flags;
        uint8_t      signature;
        le<uint32_t> serial;
        char         label[11];
        char         system_identifier_string[8];
    } PACKED;

    struct fat_ebpb32 {
        le<uint32_t>  fat_sector_count;
        le<uint16_t>  flags;
        le<uint16_t>  version;
        le<cluster_t> root_cluster;
        le<uint16_t>  fsinfo_sector;
        le<uint16_t>  backup_sector;
        char          zero[12];
        uint8_t       drive_number;
        uint8_t       nt_flags;
        uint8_t       signature;
        le<uint32_t>  serial;
        char          label[11];
        char          system_identifier_string[8];
    } PACKED;

    struct fat_bpb {
        uint8_t code[3];
        char    oem[8];

        le<uint16_t> bytes_per_sector;
        uint8_t      sectors_per_cluster;
        le<uint16_t> reserved_sector_count;
        uint8_t      fat_count;
        le<uint16_t> directory_entries;
        le<uint16_t> total_sector_count;
        uint8_t      media_descriptor_type;
        le<uint16_t> fat_sector_count;
        le<uint16_t> sectors_per_track;
        le<uint16_t> head_count;
        le<uint32_t> hidden_sector_count;
        le<uint32_t> large_sector_count;

        union {
            fat_ebpb16 ebpb16;
            fat_ebpb32 ebpb32;

            uint8_t bootbong[476];
        };
    } PACKED;

    struct fat_time {
        uint8_t hour : 5;
        uint8_t minutes : 6;
        uint8_t seconds : 5;
    } PACKED;

    struct fat_date {
        uint8_t year : 7;
        uint8_t month : 4;
        uint8_t day : 5;
    } PACKED;

    struct fat_dirent {
        union {
            char filename[11];
            struct {
                char name[8];
                char extension[3];
            };
        };

        uint8_t attributes;
        uint8_t nt_reserved;

        uint8_t  second_tenths;
        fat_time create_time;
        fat_date create_date;

        fat_date access_date;

        le<uint16_t> first_cluster_hi;

        fat_time modify_time;
        fat_date modify_date;

        le<uint16_t> first_cluster_lo;
        le<uint32_t> size;
    } PACKED;

    struct fat_dirent_lfn {
        uint8_t  order : 5;
        uint8_t  info : 3;
        uint8_t  chars0[10];
        uint8_t  attribute;
        uint8_t  long_entry_type;
        uint8_t  checksum;
        uint8_t  chars1[12];
        uint16_t zero;
        uint8_t  chars2[4];
    } PACKED;

    static_assert(sizeof(fat_dirent) == 32);
    static_assert(sizeof(fat_dirent_lfn) == 32);
}