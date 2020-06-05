#pragma once

#include "aex/math.hpp"
#include "aex/mem/heap.hpp"
#include "aex/printk.hpp"
#include "aex/string.hpp"

namespace AEX::Mem {
    template <typename T, int chunk_count = 16, int minimum_allocation = -1>
    class Vector {
      public:
        Vector() = default;

        ~Vector() {
            if (_array)
                delete _array;
        }

        template <typename... T2>
        Vector(T2... rest) {
            pushRecursive(rest...);
        }

        T& operator[](int index) {
            if (index < 0 || index >= _count)
                return _array[0];

            return _array[index];
        }

        T at(int index) {
            if (index < 0 || index >= _count)
                return _array[0];

            return _array[index];
        }

        int pushBack(T val) {
            _count++;

            int new_mem_count = int_ceil(_count, chunk_count);
            if (new_mem_count != _mem_count) {
                _mem_count = new_mem_count;
                _array     = (T*) Heap::realloc((void*) _array, _mem_count * sizeof(T));
            }

            _array[_count - 1] = val;
            return _count - 1;
        }

        void erase(int index) {
            if (index < 0 || index >= _count)
                return;

            _array[index].~T();
            _count--;

            int copy_amount = _mem_count - index;
            memcpy(&_array[index], &_array[index + 1], copy_amount * sizeof(T));

            deallocCheck();
        }

        void clear() {
            for (int i = 0; i < _count; i++)
                _array[i].~T();

            _count = 0;

            deallocCheck();

            if (_array)
                memset(_array, '\0', _mem_count * sizeof(T));
        }

        int count() {
            return _count;
        }

      private:
        int _mem_count = 0;
        int _count     = 0;

        T* _array = nullptr;

        template <typename T1>
        void pushRecursive(T1 bong) {
            pushBack(bong);
        }

        template <typename T1, typename... T2>
        void pushRecursive(T1 bong, T2... rest) {
            pushBack(bong);
            pushRecursive(rest...);
        }

        inline void deallocCheck() {
            int new_mem_count = int_ceil(_count, chunk_count);
            if (new_mem_count != _mem_count && new_mem_count > minimum_allocation) {
                _mem_count = new_mem_count;
                _array     = (T*) Heap::realloc((void*) _array, _mem_count * sizeof(T));
            }
        }
    };
}