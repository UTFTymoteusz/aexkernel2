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

        void read(uint8_t* buffer, int len);
        void write(const uint8_t* buffer, int len);

        int readAvailable();
        int writeAvailable();

      private:
        Spinlock   _lock;
        IPC::Event _event;

        uint8_t* _buffer;
        int      _size;

        int _readPos  = 0;
        int _writePos = 0;

        int findDistance(int a, int b);
    };
}