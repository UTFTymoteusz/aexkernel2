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
         */
        void wait();

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

        Mem::Vector<Mem::SmartPointer<Proc::Thread>> _tiddies;

        bool _defunct = false;
    };
}