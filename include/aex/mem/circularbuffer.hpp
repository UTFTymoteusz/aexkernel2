#pragma once

#include "aex/assert.hpp"
#include "aex/ipc/event.hpp"
#include "aex/spinlock.hpp"
#include "aex/utility.hpp"

namespace AEX::Mem {
    template <typename T = char, bool Interruptible = false>
    class API CircularBuffer {
        public:
        CircularBuffer() {}

        CircularBuffer(int count) {
            m_buffer = new T[count];
            m_count  = count;
        }

        ~CircularBuffer() {
            delete[] m_buffer;
            m_buffer = nullptr;
        }

        T read() {
            T val;
            AEX_ASSERT(read(&val, 1));
            return val;
        }

        int read(T* buffer, int count, int quota = -1) {
            if (count <= 0)
                return 0;

            SCOPE(m_lock);

            int offset = 0;

            while (count > 0) {
                int cut = min(count, readavc());
                if (cut == 0) {
                    if (quota != -1 && offset >= quota)
                        break;

                    if (Interruptible && Proc::Thread::current()->interrupted())
                        break;

                    m_event.wait();
                    m_lock.release();

                    Proc::Thread::yield();

                    m_lock.acquire();
                }

                memcpy(buffer + offset, m_buffer + m_readPos, cut * sizeof(T));

                offset += cut;
                m_readPos += cut;
                count -= cut;

                if (m_readPos == m_count)
                    m_readPos = 0;

                m_event.raise();
            }

            return offset;
        }

        T write(T val) {
            AEX_ASSERT(write(&val, 1, true));
            return val;
        }

        int write(const T* buffer, int count, bool atomic = false) {
            if (count <= 0)
                return 0;

            SCOPE(m_lock);

            int offset = 0;

            while (atomic && dist(m_writePos, m_readPos - 1) < count) {
                if (Interruptible && Proc::Thread::current()->interrupted())
                    break;

                m_event.wait();
                m_lock.release();

                Proc::Thread::yield();

                m_lock.acquire();
            }

            while (count > 0) {
                int cut = min(count, writeavc());
                if (cut == 0) {
                    if (Interruptible && Proc::Thread::current()->interrupted())
                        break;

                    m_event.wait();
                    m_lock.release();

                    Proc::Thread::yield();

                    m_lock.acquire();
                }

                memcpy(m_buffer + m_writePos, buffer + offset, cut * sizeof(T));

                offset += cut;
                m_writePos += cut;
                count -= cut;

                if (m_writePos == m_count)
                    m_writePos = 0;

                m_event.raise();
            }

            return offset;
        }

        int readav() {
            SCOPE(m_lock);
            return dist(m_readPos, m_writePos);
        }

        int writeav() {
            SCOPE(m_lock);
            return dist(m_writePos, m_readPos - 1);
        }

        void resize(int count) {
            SCOPE(m_lock);

            m_buffer = Heap::realloc(m_buffer, count * sizeof(T));
            m_count  = count;
        }

        private:
        Spinlock   m_lock;
        IPC::Event m_event;

        T*  m_buffer = nullptr;
        int m_count  = 0;

        int m_readPos  = 0;
        int m_writePos = 0;

        int readavc() {
            return min(m_count - m_readPos, dist(m_readPos, m_writePos));
        }

        int writeavc() {
            return min(m_count - m_writePos, dist(m_writePos, m_readPos - 1));
        }

        int dist(int a, int b) {
            if (a < 0)
                a += m_count;

            if (b < 0)
                b += m_count;

            int dist = b - a;
            if (dist < 0)
                dist += m_count;

            return dist;
        }
    };
}