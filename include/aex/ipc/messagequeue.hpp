#pragma once

#include "aex/ipc/event.hpp"
#include "aex/mem/circularbuffer.hpp"
#include "aex/proc.hpp"
#include "aex/spinlock.hpp"
#include "aex/utility.hpp"

namespace AEX::IPC {
    class API MessageQueue {
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

        Spinlock            m_lock;
        Event               m_event;
        Mem::CircularBuffer m_circ_buffer = Mem::CircularBuffer(MAX_WAITING_SIZE + 1);

        int m_free = MAX_WAITING_SIZE;
    };
}