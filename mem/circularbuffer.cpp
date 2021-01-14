#include "aex/mem/circularbuffer.hpp"

#include "aex/math.hpp"
#include "aex/mem.hpp"
#include "aex/proc.hpp"
#include "aex/string.hpp"

#include <stddef.h>
#include <stdint.h>

using namespace AEX::Proc;

namespace AEX::Mem {
    CircularBuffer::CircularBuffer(int size) {
        m_buffer = new uint8_t[size];
        m_size   = size;
    }

    CircularBuffer::~CircularBuffer() {
        delete m_buffer;
        m_buffer = nullptr;
    }

    int CircularBuffer::read(void* buffer, int len) {
        if (len <= 0)
            return 0;

        m_lock.acquire();

        int offset = 0;

        while (len > 0) {
            int clen = min(len, readAvailableCut());
            if (clen == 0) {
                if (writeAvailable() < len && Thread::current()->interrupted()) {
                    m_lock.release();
                    return offset;
                }

                m_event.wait();
                m_lock.release();

                Thread::yield();

                m_lock.acquire();
                continue;
            }

            memcpy((uint8_t*) buffer + offset, m_buffer + m_readPos, clen);

            m_readPos += clen;
            m_event.raise();

            offset += clen;
            len -= clen;

            if (m_readPos == m_size)
                m_readPos = 0;
        }

        m_lock.release();
        return offset;
    }

    int CircularBuffer::write(const void* buffer, int len) {
        if (len <= 0)
            return 0;

        m_lock.acquire();

        int offset = 0;

        while (len > 0) {
            int clen = min(len, writeAvailableCut());
            if (clen == 0) {
                if (writeAvailable() < len && Thread::current()->interrupted()) {
                    m_lock.release();
                    return offset;
                }

                m_event.wait();
                m_lock.release();

                Thread::yield();

                m_lock.acquire();
                continue;
            }

            memcpy(m_buffer + m_writePos, (uint8_t*) buffer + offset, clen);

            m_writePos += clen;
            m_event.raise();

            offset += clen;
            len -= clen;

            if (m_writePos == m_size)
                m_writePos = 0;
        }

        m_lock.release();
        return offset;
    }

    int CircularBuffer::readAvailable() {
        return min(m_size, findDistance(m_readPos, m_writePos));
    }

    int CircularBuffer::writeAvailable() {
        return min(m_size, findDistance(m_writePos, m_readPos - 1));
    }

    int CircularBuffer::getReadPos() {
        return m_readPos;
    }

    int CircularBuffer::getWritePos() {
        return m_writePos;
    }

    void CircularBuffer::resize(int new_size) {
        m_lock.acquire();

        m_readPos  = 0;
        m_writePos = 0;

        m_buffer = Heap::realloc(m_buffer, new_size);

        m_lock.release();
    }

    int CircularBuffer::findDistance(int a, int b) {
        if (a < 0)
            a += m_size;

        if (b < 0)
            b += m_size;

        int c = b - a;

        if (c < 0)
            c += m_size;

        return c;
    }

    int CircularBuffer::readAvailableCut() {
        int len = m_size;

        len = min(len, m_size - m_readPos);
        len = min(len, findDistance(m_readPos, m_writePos));

        return len;
    }

    int CircularBuffer::writeAvailableCut() {
        int len = m_size;

        len = min(len, m_size - m_writePos);
        len = min(len, findDistance(m_writePos, m_readPos - 1));

        return len;
    }
}