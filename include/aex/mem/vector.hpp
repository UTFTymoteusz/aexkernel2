#pragma once

#include "aex/mem/heap.hpp"
#include "aex/printk.hpp"
#include "aex/string.hpp"

namespace AEX {
    template <typename T>
    class Vector {
      public:
        Vector() = default;

        template <typename... T2>
        Vector(T2... rest) {
            pushRecursive(rest...);
        }

        T at(int index) {
            if (index < 0 || index >= _count)
                return _array[0];

            return _array[index];
        }

        void pushBack(T val) {
            _count++;
            _array = (T*) Heap::realloc((void*) _array, _count * sizeof(T));

            _array[_count - 1] = val;
        }

        void erase(int index) {
            if (index < 0 || index >= _count)
                return;

            _count--;

            int copy_amount = _count - index;
            memcpy(&_array[index], &_array[index + 1], copy_amount * sizeof(T));

            _array = (T*) Heap::realloc((void*) _array, _count * sizeof(T));
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