#pragma once

#include "aex/math.hpp"
#include "aex/mem/heap.hpp"
#include "aex/string.hpp"
#include "aex/utility.hpp"

namespace AEX::Mem {
    template <typename T, int chunk_count = 16, int minimum_allocation = -1>
    class API Vector {
        public:
        Vector() = default;

        ~Vector() {
            clear();

            if (m_array)
                delete m_array;
        }

        template <typename... T2>
        Vector(T2... rest) {
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

        int push(T val) {
            m_count++;

            int new_mem_count = int_ceil(m_count, chunk_count);
            if (new_mem_count != m_mem_count) {
                m_mem_count = new_mem_count;
                m_array     = (T*) Heap::realloc((void*) m_array, m_mem_count * sizeof(T));
            }

            m_array[m_count - 1] = val;
            return m_count - 1;
        }

        void erase(int index) {
            if (index < 0 || index >= m_count)
                return;

            m_array[index].~T();
            m_count--;

            int copy_amount = m_mem_count - index;
            memcpy(&m_array[index], &m_array[index + 1], copy_amount * sizeof(T));

            deallocCheck();
        }

        void clear() {
            for (int i = 0; i < m_count; i++)
                m_array[i].~T();

            m_count = 0;

            deallocCheck();

            if (m_array)
                memset(m_array, '\0', m_mem_count * sizeof(T));
        }

        int count() {
            return m_count;
        }

        T* get() {
            return m_array;
        }

        T* begin() {
            return &m_array[0];
        }

        T* end() {
            return &m_array[m_count];
        }

        private:
        int m_mem_count = 0;
        int m_count     = 0;

        T* m_array = nullptr;

        template <typename T1>
        void pushRecursive(T1 bong) {
            push(bong);
        }

        template <typename T1, typename... T2>
        void pushRecursive(T1 bong, T2... rest) {
            push(bong);
            pushRecursive(rest...);
        }

        inline void deallocCheck() {
            int new_mem_count = int_ceil(m_count, chunk_count);
            if (new_mem_count != m_mem_count && new_mem_count > minimum_allocation) {
                m_mem_count = new_mem_count;
                m_array     = (T*) Heap::realloc((void*) m_array, m_mem_count * sizeof(T));
            }
        }
    };
}