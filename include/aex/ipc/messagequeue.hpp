#pragma once

#include "aex/ipc/event.hpp"
#include "aex/mem/circularbuffer.hpp"
#include "aex/proc/process.hpp"
#include "aex/spinlock.hpp"

namespace AEX::IPC {
    class MessageQueue {
      public:
        static constexpr auto MAX_WAITING_SIZE = 65536 - 1;

        MessageQueue();

        template <typename T>
        int readArray(T* buffer, int count) {
            return readMessage((void*) buffer, count * sizeof(T)) / sizeof(T);
        }

        template <typename T>
        T readObject() {
            T ret;
            readMessage(&ret, sizeof(T));

            return ret;
        }

        int readMessage(void* buffer, int len);

        template <typename T>
        void writeArray(const T* ptr, int count) {
            writeMessage((void*) ptr, count * sizeof(T));
        }

        template <typename T>
        void writeObject(const T& data) {
            writeMessage(&data, sizeof(T));
        }

        void writeMessage(const void* ptr, int len);

      private:
        struct message_header {
            Proc::pid_t pid;
            int         len;
        };

        Spinlock            _lock;
        Event               _event;
        Mem::CircularBuffer _circ_buffer = Mem::CircularBuffer(MAX_WAITING_SIZE + 1);

        int _free = MAX_WAITING_SIZE;
    };
}