#pragma once

#include "aex/mem/heap.hpp"
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
            if (index < 0 || index >= m_count)
                return m_array[0];

            return m_array[index];
        }

        T at(int index) {
            if (index < 0 || index >= m_count)
                return m_array[0];

            return m_array[index];
        }

        int pushBack(T val) {
            for (int i = 0; i < m_count; i++) {
                if (m_array[i] == nullboi) {
                    m_array[i] = val;
                    return i;
                }
            }

            m_count++;
            m_array = (T*) Heap::realloc((void*) m_array, m_count * sizeof(T));

            memcpy(&m_array[m_count - 1], &val, sizeof(T));

            return m_count - 1;
        }

        void erase(int index) {
            if (index < 0 || index >= m_count)
                return;

            m_array[index] = nullboi;

            if (index == m_count - 1) {
                m_count--;
                m_array = (T*) Heap::realloc((void*) m_array, m_count * sizeof(T));
            }
        }

        int count() {
            return m_count;
        }

        private:
        int m_count = 0;
        T*  m_array = nullptr;

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