#pragma once

#include "kernel/printk.hpp"
#include "kernel/spinlock.hpp"
#include "mem/heap.hpp"

#include <new>

namespace AEX {
    template <typename T>
    class RCPArray {
      public:
        class Pointer {
          public:
            Pointer(T* ptr, Spinlock* lock) {
                _refs    = new int(1);
                _ptr     = ptr;
                _lock    = lock;
                _present = true;

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
                    printk("lets get rid of da object\n");

                    delete _ptr;
                    delete _refs;
                }

                _lock->release();
            }

            T& operator*() { return *_ptr; }

            T* operator->() { return _ptr; }

            Pointer& operator=(const Pointer& sp) {
                if (this == &sp)
                    return *this;

                _lock->acquire();

                (*_refs)++;
                printk("reffed t0x%p, 0x%p (%i)\n", this, _refs, *_refs);

                _lock->release();

                return *this;
            }

            bool isPresent() { return _ptr != nullptr && _present; }

            void remove() {
                _lock->acquire();

                (*_refs)--;
                _present = false;

                if ((*_refs) == 0) {
                    printk("lets get rid of da pointer entirely\n");

                    delete _ptr;
                    delete _refs;
                }

                _lock->release();
            }

          private:
            int*      _refs    = nullptr;
            T*        _ptr     = nullptr;
            bool      _present = true;
            Spinlock* _lock;
        };

        /*Pointer& operator[](int index) {
            printk("indexed\n");

            if (index < 0 || index >= _pointerCount) {
                // return sum thing that returns nullptr
                printk("aa!!!\n");
            }
            return _pointers[index];
        }*/

        Pointer get(int index) { return *&_pointers[index]; }

        int addRef(T* ptr) {
            _lock.acquire();

            int index = findSlotOrMakeSlot();

            new ((void*) &(_pointers[index])) Pointer(ptr, &_lock);

            _lock.release();

            return index;
        }

        int count() { return _pointerCount; }

      private:
        Spinlock _lock;

        int      _pointerCount = 0;
        Pointer* _pointers     = nullptr;

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