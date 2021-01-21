#pragma once

#include "aex/errno.hpp"
#include "aex/fs/file.hpp"
#include "aex/mem/paging.hpp"
#include "aex/mutex.hpp"
#include "aex/optional.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Proc {
    class Process;
}

namespace AEX::Mem {
    enum prot_flags_t {
        PROT_NONE  = 0x00,
        PROT_READ  = 0x01,
        PROT_WRITE = 0x02,
        PROT_EXEC  = 0x04,
    };

    enum mmap_flags_t {
        MAP_NONE      = 0x00,
        MAP_PRIVATE   = 0x01,
        MAP_SHARED    = 0x02,
        MAP_FIXED     = 0x04,
        MAP_ANONYMOUS = 0x10,
    };

    class MMapRegion {
        public:
        void*  start;
        size_t len;

        MMapRegion(Pagemap* pagemap, void* addr, size_t len);
        virtual ~MMapRegion();

        virtual error_t read(void* dst, int64_t offset, uint32_t count);

        protected:
        optional<FS::File_SP> m_file;
        Pagemap*              m_pagemap;
    };

    class FileBackedMMapRegion : public MMapRegion {
        public:
        FileBackedMMapRegion(Pagemap* pagemap, void* addr, size_t len, FS::File_SP file,
                             int64_t offset);
        ~FileBackedMMapRegion();

        error_t read(void* dst, int64_t offset, uint32_t count);

        private:
        struct cache_slot {
            int32_t  id         = -1;
            uint8_t* buffer     = nullptr;
            void*    referenced = nullptr;
        };

        static constexpr auto CACHE_SLOTS = 8;

        Mutex m_lock;

        int64_t m_offset;

        int        cache_ptr = 0;
        cache_slot cache[CACHE_SLOTS];

        int findSlot(int32_t id);
    };

    optional<void*> mmap(Proc::Process* process, void* addr, size_t len, int prot, int flags,
                         FS::File_SP file = FS::File_SP::getNull(), int64_t offset = 0);
    error_t         munmap(Proc::Process* process, void* addr, size_t len);

    Mem::MMapRegion* find_mmap_region(Proc::Process* process, void* addr);
}