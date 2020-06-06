#pragma once

#include "aex/ipc/event.hpp"
#include "aex/spinlock.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Mem {
    class CircularBuffer {
        public:
        CircularBuffer(int size);
        ~CircularBuffer();

        void read(void* buffer, int len);
        void write(const void* buffer, int len);

        int readAvailable();
        int writeAvailable();

        int getReadPos();
        int getWritePos();

        private:
        Spinlock   _lock;
        IPC::Event _event;

        uint8_t* _buffer = nullptr;
        int      _size   = 0;

        int _readPos  = 0;
        int _writePos = 0;

        int findDistance(int a, int b);

        int readAvailableCut();
        int writeAvailableCut();
    };
}