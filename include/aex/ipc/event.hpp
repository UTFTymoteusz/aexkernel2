#pragma once

#include "aex/mem/smartptr.hpp"
#include "aex/mem/vector.hpp"
#include "aex/proc/thread.hpp"
#include "aex/spinlock.hpp"

namespace AEX::IPC {
    class Event {
        public:
        /**
         * Blocks the currently executing thread on this event.
         * @param ms_timeout If this is more than 0, the thread will automatically wake up after the
         * specified amount of milliseconds.
         */
        void wait(int ms_timeout = 0);

        /**
         * Unblocks all threads on waiting on this event.
         * @returns The amount of threads that had been unblocked.
         */
        int raise();

        /**
         * Unblocks all threads on waiting on this event and marks it as defunct, disallowing any
         * future blocks.
         * @returns The amount of threads that had been unblocked.
         */
        int defunct();

        private:
        Spinlock _lock;

        Mem::Vector<Mem::SmartPointer<Proc::Thread>, 8, 8> _tiddies;

        bool _defunct = false;
    };

    class SimpleEvent {
        public:
        /**
         * Blocks the currently executing thread on this event.
         * @param ms_timeout If this is more than 0, the thread will automatically wake up after the
         * specified amount of milliseconds.
         */
        void wait(int ms_timeout = 0);

        /**
         * Unblocks the thread waiting on this event
         */
        void raise();

        /**
         * Unblocks the thread waiting on this event and marks it as defunct, disallowing any
         * future blocks.
         */
        void defunct();

        private:
        Spinlock _lock;

        Mem::SmartPointer<Proc::Thread> _tiddie;

        bool _defunct = false;
    };
}