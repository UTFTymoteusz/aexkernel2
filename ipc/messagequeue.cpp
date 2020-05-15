#include "aex/ipc/messagequeue.hpp"

#include "aex/math.hpp"
#include "aex/proc/thread.hpp"


using namespace AEX::Proc;

namespace AEX::IPC {
    MessageQueue::MessageQueue() {}

    int MessageQueue::readMessage(void* ptr, int len) {
        int total_len = sizeof(len) + sizeof(message_header);
        if (total_len > MAX_WAITING_SIZE)
            return 0;

        _lock.acquire();

        while ((MAX_WAITING_SIZE - _free) < total_len) {
            _event.wait();
            _lock.release();

            Thread::yield();

            _lock.acquire();
        }

        message_header header;
        _circ_buffer.read((uint8_t*) &header, sizeof(header));
        _circ_buffer.read((uint8_t*) ptr, len);

        int left = header.len - len;
        while (left > 0) {
            uint8_t buffer[32];
            _circ_buffer.read(buffer, min(left, 32));

            left -= min(left, 32);
        }

        _lock.release();
        _event.raise();

        return min(header.len, len);
    }

    void MessageQueue::writeMessage(const void* ptr, int len) {
        int total_len = sizeof(len) + sizeof(message_header);
        if (total_len > MAX_WAITING_SIZE)
            return;

        _lock.acquire();

        while (_free < total_len) {
            _event.wait();
            _lock.release();

            Thread::yield();

            _lock.acquire();
        }

        _free -= total_len;

        message_header header;
        header.pid = Thread::getCurrentThread()->getProcess()->pid;

        _circ_buffer.write((uint8_t*) &header, sizeof(header));
        _circ_buffer.write((uint8_t*) ptr, len);

        _lock.release();
        _event.raise();
    }
}