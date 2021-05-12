#pragma once

#include "aex/kpanic.hpp"
#include "aex/mem/atomic.hpp"
#include "aex/spinlock.hpp"
#include "aex/utility.hpp"

namespace AEX::Mem {
    struct API ref_counter {
        int refs;

        ref_counter(int srefs) {
            refs = srefs;
        }

        void increment() {
            atomic_add(&refs, 1);
        }

        bool decrement() {
            return atomic_sub_fetch(&refs, 1) == 0;
        }

        int ref_count() {
            return atomic_read(&refs);
        }
    };

    struct API sp_shared : ref_counter {
        bool cleaning;

        sp_shared(int refs) : ref_counter(refs) {}
    };

    template <typename T>
    class API SmartPointer {
        public:
        SmartPointer() {
            m_val  = nullptr;
            m_refs = nullptr;
        }

        SmartPointer(T* val) {
            m_val  = val;
            m_refs = new sp_shared(1);
        }

        SmartPointer(T* val, sp_shared* sp_shared) {
            m_val  = val;
            m_refs = sp_shared;
        }

        SmartPointer(const SmartPointer& sp) {
            m_val  = sp.m_val;
            m_refs = sp.m_refs;

            if (m_refs)
                m_refs->increment();
        }

        template <typename T2>
        SmartPointer(const SmartPointer<T2>& sp) {
            m_val  = (T*) sp.m_val;
            m_refs = sp.m_refs;

            if (m_refs)
                m_refs->increment();
        }

        ~SmartPointer() {
            if (!m_refs)
                return;

            if (m_refs->decrement())
                cleanup();
        }

        T& operator*() {
            return *m_val;
        }

        T* operator->() {
            return m_val;
        }

        SmartPointer& operator=(const SmartPointer& sp) {
            if (this == &sp)
                return *this;

            if (m_refs && m_refs->decrement())
                cleanup();

            m_val  = sp.m_val;
            m_refs = sp.m_refs;

            if (m_refs)
                m_refs->increment();

            return *this;
        }

        static auto null() {
            return SmartPointer<T>(nullptr, nullptr);
        }

        T* get() {
            return m_val;
        }

        int refCount() {
            return m_refs->ref_count();
        }

        bool isValid() {
            return m_val != nullptr;
        }

        void defuse() {
            m_refs = nullptr;
        }

        void decrement() {
            m_refs->decrement();
        }

        operator bool() {
            return isValid();
        }

        private:
        sp_shared* m_refs;
        T*         m_val;

        void cleanup() {
            if (m_val)
                delete m_val;

            if (m_refs)
                delete m_refs;

            m_val  = nullptr;
            m_refs = nullptr;
        }

        template <typename>
        friend class SmartPointer;
    };
}