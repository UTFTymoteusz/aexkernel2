#include "aex/mem/circularbuffer.hpp"

#include "aex/debug.hpp"
#include "aex/math.hpp"
#include "aex/mem.hpp"
#include "aex/proc.hpp"
#include "aex/string.hpp"

#include <stddef.h>
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

    void CircularBuffer::read(void* buffer, int len) {
        _lock.acquire();

        int offset = 0;

        while (len > 0) {
            int clen = min(len, readAvailableCut());
            if (clen == 0) {
                _event.wait();
                _lock.release();

                Thread::yield();

                _lock.acquire();

                continue;
            }

            memcpy((uint8_t*) buffer + offset, _buffer + _readPos, clen);

            _readPos += clen;
            _event.raise();

            offset += clen;
            len -= clen;

            if (_readPos == _size)
                _readPos = 0;
        }

        _lock.release();
    }

    void CircularBuffer::write(const void* buffer, int len) {
        _lock.acquire();

        int offset = 0;

        while (len > 0) {
            int clen = min(len, writeAvailableCut());
            if (clen == 0) {
                _event.wait();
                _lock.release();

                Thread::yield();

                _lock.acquire();

                continue;
            }

            memcpy(_buffer + _writePos, (uint8_t*) buffer + offset, clen);

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
        return min(_size, findDistance(_readPos, _writePos));
    }

    int CircularBuffer::writeAvailable() {
        return min(_size, findDistance(_writePos, _readPos - 1));
    }

    int CircularBuffer::getReadPos() {
        return _readPos;
    }

    int CircularBuffer::getWritePos() {
        return _writePos;
    }

    int CircularBuffer::findDistance(int a, int b) {
        if (a < 0)
            a += _size;

        if (b < 0)
            b += _size;

        int c = b - a;

        if (c < 0)
            c += _size;

        return c;
    }

    int CircularBuffer::readAvailableCut() {
        int len = _size;

        len = min(len, _size - _readPos);
        len = min(len, findDistance(_readPos, _writePos));

        return len;
    }

    int CircularBuffer::writeAvailableCut() {
        int len = _size;

        len = min(len, _size - _writePos);
        len = min(len, findDistance(_writePos, _readPos - 1));

        return len;
    }
}