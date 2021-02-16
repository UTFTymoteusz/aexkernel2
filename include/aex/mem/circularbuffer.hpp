#pragma once

#include "aex/ipc/event.hpp"
#include "aex/spinlock.hpp"
#include "aex/utility.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Mem {
    class API CircularBuffer {
        public:
        CircularBuffer(int size);
        ~CircularBuffer();

        int read(void* buffer, int len);
        int write(const void* buffer, int len);

        int readAvailable();
        int writeAvailable();

        int getReadPos();
        int getWritePos();

        void resize(int new_size);

        private:
        Spinlock   m_lock;
        IPC::Event m_event;

        uint8_t* m_buffer = nullptr;
        int      m_size   = 0;

        int m_readPos  = 0;
        int m_writePos = 0;

        int findDistance(int a, int b);

        int readAvailableCut();
        int writeAvailableCut();
    };
}