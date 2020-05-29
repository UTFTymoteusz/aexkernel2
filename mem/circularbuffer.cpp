#include "aex/mem/circularbuffer.hpp"

#include "aex/debug.hpp"
#include "aex/math.hpp"
#include "aex/mem/vmem.hpp"
#include "aex/proc/thread.hpp"
#include "aex/string.hpp"

#include <stdint.h>

using namespace AEX::Proc;

namespace AEX::Mem {
    CircularBuffer::CircularBuffer(int size) {
        _buffer = new uint8_t[size];
        _size   = size;
    }

    CircularBuffer::~CircularBuffer() {
        delete _buffer;
    }

    void CircularBuffer::read(uint8_t* buffer, int len) {
        _lock.acquire();

        int offset = 0;

        while (len > 0) {
            int clen = min(len, readAvailable());
            if (clen == 0) {
                _event.wait();
                _lock.release();

                Thread::yield();

                _lock.acquire();

                continue;
            }

            memcpy(buffer + offset, _buffer + _readPos, clen);

            _readPos += clen;
            _event.raise();

            offset += clen;
            len -= clen;

            if (_readPos == _size)
                _readPos = 0;
        }

        _lock.release();
    }

    void CircularBuffer::write(const uint8_t* buffer, int len) {
        _lock.acquire();

        int offset = 0;

        while (len > 0) {
            int clen = min(len, writeAvailable());
            if (clen == 0) {
                _event.wait();
                _lock.release();

                Thread::yield();

                _lock.acquire();

                continue;
            }

            memcpy(_buffer + _writePos, buffer + offset, clen);

            _writePos += clen;
            _event.raise();

            offset += clen;
            len -= clen;

            if (_writePos == _size)
                _writePos = 0;
        }

        _lock.release();
    }

    int CircularBuffer::readAvailable() {
        int len = _size;

        len = min(len, _size - _readPos);
        len = min(len, findDistance(_readPos, _writePos));

        return len;
    }

    int CircularBuffer::writeAvailable() {
        int len = _size;

        len = min(len, _size - _writePos);
        len = min(len, findDistance(_writePos, _readPos - 1));

        return len;
    }

    int CircularBuffer::findDistance(int a, int b) {
        if (a < 0)
            a += _size;

        if (b < 0)
            b += _size;

        int c = a - b;

        if (c < 0)
            c *= -1;

        return c;
    }
}