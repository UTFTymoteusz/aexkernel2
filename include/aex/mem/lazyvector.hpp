#pragma once

#include "aex/mem/heap.hpp"
#include "aex/optional.hpp"
#include "aex/string.hpp"

namespace AEX::Mem {
    template <typename T>
    class LazyVector {
        public:
        LazyVector() = default;

        template <typename... T2>
        LazyVector(T2... rest) {
            pushRecursive(rest...);
        }

        T& operator[](int index) {
            if (index < 0 || index >= m_count)
                return m_array[0].value;

            return m_array[index].value;
        }

        T at(int index) {
            if (index < 0 || index >= m_count)
                return m_array[0].value;

            return m_array[index].value;
        }

        bool present(int index) {
            if (index < 0 || index >= m_count)
                return false;

            return m_array[index].has_value;
        }

        int push(T val) {
            m_rcount++;

            for (int i = 0; i < m_count; i++) {
                if (m_array[i].has_value)
                    continue;

                m_array[i].value     = val;
                m_array[i].has_value = true;

                return i;
            }

            m_count++;
            m_array = (optional<T>*) Heap::realloc((void*) m_array, m_count * sizeof(T));

            m_array[m_count - 1].value     = val;
            m_array[m_count - 1].has_value = true;

            return m_count - 1;
        }

        void erase(int index) {
            if (index < 0 || index >= m_count)
                return;

            m_rcount--;

            m_array[index].value     = {};
            m_array[index].has_value = false;

            int prev = m_count;

            while (m_count > 0 && m_array[m_count - 1].has_value)
                m_count--;

            if (m_count != prev)
                m_array = (optional<T>*) Heap::realloc((void*) m_array, m_count * sizeof(T));
        }

        int count() {
            return m_count;
        }

        int realCount() {
            return m_rcount;
        }

        private:
        int          m_count  = 0;
        int          m_rcount = 0;
        optional<T>* m_array  = nullptr;

        template <typename T1>
        void pushRecursive(T1 bong) {
            push(bong);
        }

        template <typename T1, typename... T2>
        void pushRecursive(T1 bong, T2... rest) {
            push(bong);
            pushRecursive(rest...);
        }
    };
}