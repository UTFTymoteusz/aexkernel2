#pragma once

#include "aex/mem/smartptr.hpp"
#include "aex/mem/vector.hpp"
#include "aex/proc/thread.hpp"
#include "aex/spinlock.hpp"
#include "aex/utility.hpp"

namespace AEX::IPC {
    class API Event {
        public:
        /**
         * Blocks the currently executing thread on this event.
         * @param ms_timeout If this is more than 0, the thread will automatically wake up after the
         * specified amount of milliseconds.
         **/
        void wait(int ms_timeout = 0);

        /**
         * Unblocks all threads on waiting on this event.
         * @returns The amount of threads that had been unblocked.
         **/
        int raise();

        /**
         * Unblocks all threads on waiting on this event and marks it as defunct, disallowing any
         * future blocks.
         * @returns The amount of threads that had been unblocked.
         **/
        int defunct();

        /**
         * Removes the currently executing thread from this event.
         **/
        void nevermind();

        private:
        Spinlock m_lock;

        Mem::Vector<Proc::Thread*, 4, 4> m_tiddies;

        bool m_defunct = false;
    };

    class API SimpleEvent {
        public:
        /**
         * Blocks the currently executing thread on this event.
         * @param ms_timeout If this is more than 0, the thread will automatically wake up after the
         * specified amount of milliseconds.
         **/
        void wait(int ms_timeout = 0);

        /**
         * Unblocks the thread waiting on this event
         **/
        void raise();

        /**
         * Unblocks the thread waiting on this event and marks it as defunct, disallowing any
         * future blocks.
         **/
        void defunct();

        private:
        Spinlock m_lock;

        Proc::Thread* m_tiddie;

        bool m_defunct = false;
    };
}