#pragma once

#include "aex/mem/heap.hpp"
#include "aex/mem/smartptr.hpp"

namespace AEX::Mem {
    template <typename T>
    class SmartArray {
        public:
        class Iterator {
            public:
            Iterator(SmartArray<T>* base, int start = 0) {
                _index = start;
                _base  = base;

                _lock = &base->_lock;
            }

            T* next() {
                if (!_base->_elements)
                    return nullptr;

                while (_index < _base->count()) {
                    auto ptr = _base->get(_index);
                    if (!ptr) {
                        _index++;
                        continue;
                    }

                    _current = ptr;
                    _index++;

                    return _current.get();
                }

                return nullptr;
            }

            int index() {
                return _index - 1;
            }

            SmartPointer<T> get_ptr() {
                return _current;
            }

            private:
            int _index = 0;

            Spinlock*      _lock;
            SmartArray<T>* _base;

            SmartPointer<T> _current = SmartPointer<T>::getNull();
        };

        SmartPointer<T> get(int index) {
            auto scopeLock = ScopeSpinlock(_lock);

            if (index < 0 || index >= _element_count)
                return SmartPointer<T>(nullptr, nullptr);

            if (!_elements[index].shared)
                return SmartPointer<T>(nullptr, nullptr);

            _elements[index].shared->increment();

            return SmartPointer<T>(_elements[index].ptr, _elements[index].shared);
        }

        int count() {
            return _element_count;
        }

        int addRef(T* ptr) {
            auto scopeLock = ScopeSpinlock(_lock);

            int index = findSlotOrMakeSlot();
            if (index == -1)
                return -1;

            _elements[index] = element(ptr);

            return index;
        }

        int addRef(T* ptr, sp_shared* counter) {
            auto scopeLock = ScopeSpinlock(_lock);

            int index = findSlotOrMakeSlot();
            if (index == -1)
                return -1;

            _elements[index] = element(ptr, counter);

            return index;
        }

        void remove(int index) {
            auto scopeLock = ScopeSpinlock(_lock);

            if (index < 0 || index >= _element_count)
                return;

            _elements[index].die();
        }

        Iterator getIterator(int start = 0) {
            return Iterator(this, start);
        }

        private:
        struct element {
            T*         ptr;
            sp_shared* shared;

            element(T* ptr) {
                this->ptr    = ptr;
                this->shared = new sp_shared(1);
            }

            element(T* ptr, sp_shared* shared) {
                this->ptr    = ptr;
                this->shared = shared;
            }

            void die() {
                if (shared && shared->decrement()) {
                    if (ptr)
                        delete ptr;

                    delete shared;
                }

                ptr    = nullptr;
                shared = nullptr;
            }
        };

        Spinlock _lock;

        int      _element_count = 0;
        element* _elements      = nullptr;

        int findSlotOrMakeSlot() {
            for (int i = 0; i < _element_count; i++) {
                if (!_elements[i].ptr || !_elements[i].shared ||
                    _elements[i].shared->ref_count() == 0) {
                    // printk("!present slot: %i\n", i);
                    return i;
                }
            }

            _elements = (element*) Heap::realloc(_elements, (_element_count + 1) * sizeof(element));
            _element_count++;

            return _element_count - 1;
        }
    };
}