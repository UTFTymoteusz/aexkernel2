#include "aex/ipc/messagequeue.hpp"

#include "aex/math.hpp"
#include "aex/printk.hpp"
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

        // TODO: Make it work properly when the read gets interrupted
        m_circ_buffer.read(&header, sizeof(header));
        m_circ_buffer.read(ptr, len);

        int left = header.len - len;
        while (left > 0) {
            uint8_t buffer[32];
            if (m_circ_buffer.read(buffer, min(left, 32)) == 0)
                break;

            left -= min(left, 32);
        }
        // TODO END

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
        header.pid = Thread::current()->getProcess()->pid;
        header.len = len;

        // TODO: Make it work properly when the write gets interrupted
        m_circ_buffer.write(&header, sizeof(header));
        m_circ_buffer.write(ptr, len);

        m_event.raise();
        m_lock.release();
    }
}