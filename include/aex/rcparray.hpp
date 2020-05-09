#pragma once

#include "aex/kpanic.hpp"
#include "aex/mem/heap.hpp"
#include "aex/printk.hpp"
#include "aex/spinlock.hpp"

#include <new>

namespace AEX {
    template <typename T>
    class RCPArray {
      public:
        class Pointer {
          public:
            Pointer() {}

            Pointer(T* ptr, Spinlock* lock) {
                _refs    = &_refCounter;
                _ptr     = ptr;
                _lock    = lock;
                _present = true;

                _refCounter = 1;

                // printk("ctorred t0x%p, 0x%p\n", this, _refs);
            }

            Pointer(T* ptr, Spinlock* lock, int* ref_counter, bool present = true) {
                _refs    = ref_counter;
                _ptr     = ptr;
                _lock    = lock;
                _present = present;

                // printk("ctorred t0x%p, 0x%p\n", this, _refs);
            }

            Pointer(const Pointer& base) {
                base._lock->acquire();

                _refs    = base._refs;
                _ptr     = base._ptr;
                _lock    = base._lock;
                _present = base._present;

                (*_refs)++;
                // printk("reffed t0x%p, 0x%p (%i)\n", this, _refs, *_refs);

                base._lock->release();
            }

            ~Pointer() {
                _lock->acquire();

                (*_refs)--;
                // printk("delett t0x%p, 0x%p (%i)\n", this, _refs, *_refs);

                if ((*_refs) == 0) {
                    if (_ptr == nullptr)
                        kpanic("RCPArray tried to remove null");

                    printk("lets get rid of da object\n");

                    delete _ptr;
                    delete _refs;

                    _lock->release();
                }

                _lock->release();
            }

            T& operator*() {
                return *_ptr;
            }

            T* operator->() {
                return _ptr;
            }

            Pointer& operator=(const Pointer& sp) {
                if (this == &sp || !_lock)
                    return *this;

                _lock->acquire();

                (*_refs)++;

                _lock->release();

                return *this;
            }

            T* get() {
                return _ptr;
            }

            bool isPresent() {
                return _ptr != nullptr || _present;
            }

            void remove() {
                _lock->acquire();

                if (!_present) {
                    _lock->release();
                    return;
                }

                (*_refs)--;
                _present = false;

                if ((*_refs) == 0) {
                    if (_ptr == nullptr)
                        kpanic("RCPArray tried to remove null\n");

                    printk("lets get rid of da pointer entirely\n");

                    delete _ptr;
                    delete _refs;
                }

                _lock->release();
            }

          private:
            int* _refs       = nullptr;
            int  _refCounter = 1;

            T*        _ptr     = nullptr;
            bool      _present = true;
            Spinlock* _lock;
        };

        class Iterator {
          public:
            Iterator(RCPArray<T>* base, int start = 0) {
                _index = start;
                _base  = base;

                _lock = &base->_lock;
            }

            T* next() {
                if (!_base->_pointers)
                    return nullptr;

                while (_index < _base->count()) {
                    auto ptr = _base->_pointers[_index];
                    auto val = ptr.get();

                    if (!ptr.isPresent()) {
                        _index++;
                        continue;
                    }

                    _index++;

                    return val;
                }

                return nullptr;
            }

            int index() {
                return _index - 1;
            }

          private:
            int _index = 0;

            Spinlock*    _lock;
            RCPArray<T>* _base;
        };

        // Gotta make this better
        ~RCPArray() {
            delete _pointers;
        }

        Pointer& operator[](int) = delete;

        /*Pointer& operator[](int index) {
            printk("indexed\n");

            if (index < 0 || index >= _pointerCount) {
                // return sum thing that returns nullptr
                printk("aa!!!\n");
            }
            return _pointers[index];
        }*/

        Pointer get(int index) {
            if (index < 0 || index >= _pointerCount)
                return getNullPointer();

            return *&_pointers[index];
        }

        int addRef(T* ptr) {
            _lock.acquire();

            int index = findSlotOrMakeSlot();

            new ((void*) &(_pointers[index])) Pointer(ptr, &_lock);

            _lock.release();

            return index;
        }

        int count() {
            return _pointerCount;
        }

        Pointer getNullPointer() {
            _nullRefCounter = 10000000;
            return _nullPointer;
        }

        Iterator getIterator(int start = 0) {
            return Iterator(this, start);
        }

      private:
        Spinlock _lock = {};

        int      _pointerCount = 0;
        Pointer* _pointers     = nullptr;

        int     _nullRefCounter = 2137;
        Pointer _nullPointer    = Pointer(nullptr, &_lock, &_nullRefCounter, false);

        int findSlotOrMakeSlot() {
            for (int i = 0; i < _pointerCount; i++) {
                if (!_pointers[i].isPresent()) {
                    printk("!present slot: %i\n", i);
                    return i;
                }
            }

            _pointers = (Pointer*) Heap::realloc(_pointers, (_pointerCount + 1) * sizeof(Pointer));
            _pointerCount++;

            // printk("resize 0x%p (%i, %i)\n", _pointers, _pointerCount * sizeof(Pointer),
            // Heap::msize((void*) _pointers)); printk("slot: %i\n", _pointerCount - 1);

            return _pointerCount - 1;
        }
    };
}