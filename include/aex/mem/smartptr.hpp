#pragma once

#include "aex/kpanic.hpp"
#include "aex/mem/atomic.hpp"
#include "aex/spinlock.hpp"

namespace AEX::Mem {
    struct ref_counter {
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

    struct sp_shared : ref_counter {
        bool cleaning;

        sp_shared(int refs) : ref_counter(refs) {}
    };

    template <typename T>
    class SmartPointer {
        public:
        SmartPointer() {
            _val  = nullptr;
            _refs = nullptr;
        }

        SmartPointer(T* val) {
            _val  = val;
            _refs = new sp_shared(1);
        }

        SmartPointer(T* val, sp_shared* sp_shared) {
            _val  = val;
            _refs = sp_shared;
        }

        SmartPointer(const SmartPointer& sp) {
            _val  = sp._val;
            _refs = sp._refs;

            if (_refs)
                _refs->increment();
        }

        template <typename T2>
        SmartPointer(const SmartPointer<T2>& sp) {
            _val  = (T*) sp._val;
            _refs = sp._refs;

            if (_refs)
                _refs->increment();
        }

        ~SmartPointer() {
            if (!_refs)
                return;

            if (_refs->decrement())
                cleanup();
        }

        T& operator*() {
            return *_val;
        }

        T* operator->() {
            return _val;
        }

        SmartPointer& operator=(const SmartPointer& sp) {
            if (this == &sp)
                return *this;

            if (_refs && _refs->decrement())
                cleanup();

            _val  = sp._val;
            _refs = sp._refs;

            if (_refs)
                _refs->increment();

            return *this;
        }

        static auto getNull() {
            return SmartPointer<T>(nullptr, nullptr);
        }

        T* get() {
            return _val;
        }

        int refCount() {
            return _refs->ref_count();
        }

        bool isValid() {
            return _val != nullptr;
        }

        void defuse() {
            _refs = nullptr;
        }

        void decrement() {
            _refs->decrement();
        }

        operator bool() {
            return isValid();
        }

        private:
        sp_shared* _refs;
        T*         _val;

        void cleanup() {
            if (_val)
                delete _val;

            if (_refs)
                delete _refs;

            _val  = nullptr;
            _refs = nullptr;
        }

        template <typename>
        friend class SmartPointer;
    };
}