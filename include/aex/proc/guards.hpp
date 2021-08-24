#pragma once

#include "aex/proc/types.hpp"

namespace AEX::Proc {
    class API CriticalGuard {
        public:
        class API Scope {
            public:
            Scope(Thread* thread);
            ~Scope();

            private:
            Thread* m_thread = nullptr;
        };

        Scope scope() {
            return Scope(m_thread);
        }

        CriticalGuard(Thread* thread) {
            m_thread = thread;
        }

        void acquire();
        void release();

        private:
        Thread* m_thread = nullptr;
    };

    class API SignabilityGuard {
        public:
        class API Scope {
            public:
            Scope(Thread* thread);
            ~Scope();

            private:
            Thread* m_thread = nullptr;
        };

        Scope scope() {
            return Scope(m_thread);
        }

        SignabilityGuard(Thread* thread) {
            m_thread = thread;
        }

        void acquire();
        void release();

        private:
        Thread* m_thread = nullptr;
    };

    class API BusyGuard {
        public:
        class API Scope {
            public:
            Scope(Thread* thread);
            ~Scope();

            private:
            Thread* m_thread = nullptr;
        };

        Scope scope() {
            return Scope(m_thread);
        }

        BusyGuard(Thread* thread) {
            m_thread = thread;
        }

        void acquire();
        void release();

        private:
        Thread* m_thread = nullptr;
    };
}