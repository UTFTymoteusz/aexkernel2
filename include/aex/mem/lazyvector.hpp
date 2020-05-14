#pragma once

#include "aex/mem/heap.hpp"
#include "aex/printk.hpp"
#include "aex/string.hpp"

namespace AEX::Mem {
    template <typename T, T nullboi>
    class LazyVector {
      public:
        LazyVector() = default;

        template <typename... T2>
        LazyVector(T2... rest) {
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
            for (int i = 0; i < _count; i++) {
                if (_array[i] == nullboi) {
                    _array[i] = val;
                    return i;
                }
            }

            _count++;
            _array = (T*) Heap::realloc((void*) _array, _count * sizeof(T));

            _array[_count - 1] = val;

            return _count - 1;
        }

        void erase(int index) {
            if (index < 0 || index >= _count)
                return;

            _array[index] = nullboi;

            if (index == _count - 1) {
                _count--;
                _array = (T*) Heap::realloc((void*) _array, _count * sizeof(T));
            }
        }

        int count() {
            return _count;
        }

      private:
        int _count = 0;
        T*  _array = nullptr;

        template <typename T1>
        void pushRecursive(T1 bong) {
            pushBack(bong);
        }

        template <typename T1, typename... T2>
        void pushRecursive(T1 bong, T2... rest) {
            pushBack(bong);
            pushRecursive(rest...);
        }
    };
}