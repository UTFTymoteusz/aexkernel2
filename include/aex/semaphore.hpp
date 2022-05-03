#pragma once

#include "aex/utility.hpp"

namespace AEX {
    class API Semaphore {
        public:
        Semaphore(int count, int max);
        ~Semaphore();

        void acquire();
        void release();
        bool tryAcquire();

        private:
        int m_starting;
        int m_count;
        int m_max;
        int m_holder;
    };
}