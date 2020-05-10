#pragma once

#include "aex/kpanic.hpp"
#include "aex/mem/atomic.hpp"
#include "aex/printk.hpp"
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

    template <typename T>
    class SmartPointer {
      public:
        SmartPointer(T* val) {
            _val  = val;
            _refs = new ref_counter(1);
        }

        SmartPointer(T* val, ref_counter* ref_counter) {
            _val  = val;
            _refs = ref_counter;
        }

        SmartPointer(const SmartPointer& sp) {
            _val  = sp._val;
            _refs = sp._refs;

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

        T* get() {
            return _val;
        }

        int refCount() {
            return _refs->ref_count();
        }

        bool isValid() {
            return _val != nullptr;
        }

        static auto getNull() {
            return SmartPointer<T>(nullptr, nullptr);
        }

      private:
        ref_counter* _refs;
        T*           _val;

        void cleanup() {
            if (_val)
                delete _val;

            if (_refs)
                delete _refs;
        }
    };
}