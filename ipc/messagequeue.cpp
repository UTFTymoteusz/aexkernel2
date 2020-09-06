#include "aex/ipc/messagequeue.hpp"

#include "aex/math.hpp"
#include "aex/proc.hpp"

using namespace AEX::Proc;

namespace AEX::IPC {
    MessageQueue::MessageQueue() {}

    int MessageQueue::readMessage(void* ptr, int len) {
        int total_len = len + sizeof(message_header);
        if (total_len > MAX_WAITING_SIZE)
            return 0;

        m_lock.acquire();

        while ((MAX_WAITING_SIZE - m_free) < total_len) {
            m_event.wait();
            m_lock.release();

            Thread::yield();

            m_lock.acquire();
        }

        message_header header;
        m_circ_buffer.read((uint8_t*) &header, sizeof(header));
        m_circ_buffer.read((uint8_t*) ptr, len);

        int left = header.len - len;
        while (left > 0) {
            uint8_t buffer[32];
            m_circ_buffer.read(buffer, min(left, 32));

            left -= min(left, 32);
        }

        m_free += total_len;

        m_event.raise();
        m_lock.release();

        return min(header.len, len);
    }

    void MessageQueue::writeMessage(const void* ptr, int len) {
        int total_len = len + sizeof(message_header);
        if (total_len > MAX_WAITING_SIZE)
            return;

        m_lock.acquire();

        while (m_free < total_len) {
            m_event.wait();
            m_lock.release();

            Thread::yield();

            m_lock.acquire();
        }

        m_free -= total_len;

        message_header header;
        header.pid = Thread::getCurrent()->getProcess()->pid;
        header.len = len;

        m_circ_buffer.write((uint8_t*) &header, sizeof(header));
        m_circ_buffer.write((uint8_t*) ptr, len);

        m_event.raise();
        m_lock.release();
    }
}