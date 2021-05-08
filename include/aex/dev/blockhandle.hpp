#pragma once

#include "aex/mem/smartptr.hpp"
#include "aex/mutex.hpp"
#include "aex/utility.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Dev {
    class BlockDevice;

    class API BlockHandle {
        public:
        BlockHandle();
        BlockHandle(Mem::SmartPointer<BlockDevice> blkhndl);
        BlockHandle(const BlockHandle& bh);
        ~BlockHandle();

        int64_t read(void* buffer, uint64_t start, uint32_t len);
        int64_t write(const void* buffer, uint64_t start, uint32_t len);

        Mem::SmartPointer<BlockDevice> getDev();

        private:
        struct shared {
            uint8_t* m_buffer = nullptr;
            Mutex    m_mutex;

            Mem::ref_counter m_refs = Mem::ref_counter(1);

            shared() {}
        };

        shared*                        m_shared;
        Mem::SmartPointer<BlockDevice> m_dev;
        uint16_t                       m_sector_size;

        bool isAligned(const void* addr);
        bool isPerfect(uint64_t start, uint32_t len);
    };
}