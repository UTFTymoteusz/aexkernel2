#pragma once

#include "aex/mem/heap.hpp"
#include "aex/mem/smartptr.hpp"
#include "aex/utility.hpp"

namespace AEX::Mem {
    template <typename T>
    class API SmartArray {
        public:
        class Iterator {
            public:
            Iterator(SmartArray<T>* base, int start = 0) {
                m_index = start;
                m_base  = base;

                m_lock = &base->m_lock;
            }

            T* next() {
                if (!m_base->m_elements)
                    return nullptr;

                while (m_index < m_base->count()) {
                    auto ptr = m_base->get(m_index);
                    if (!ptr) {
                        m_index++;
                        continue;
                    }

                    m_current = ptr;
                    m_index++;

                    return m_current.get();
                }

                return nullptr;
            }

            int index() {
                return m_index - 1;
            }

            SmartPointer<T> get_ptr() {
                return m_current;
            }

            private:
            int m_index = 0;

            Spinlock*      m_lock;
            SmartArray<T>* m_base;

            SmartPointer<T> m_current = SmartPointer<T>::getNull();
        };

        SmartPointer<T> get(int index) {
            ScopeSpinlock scopeLock(m_lock);

            if (index < 0 || index >= m_element_count)
                return SmartPointer<T>(nullptr, nullptr);

            if (!m_elements[index].shared)
                return SmartPointer<T>(nullptr, nullptr);

            m_elements[index].shared->increment();

            return SmartPointer<T>(m_elements[index].ptr, m_elements[index].shared);
        }

        int count() {
            return m_element_count;
        }

        int addRef(T* ptr) {
            ScopeSpinlock scopeLock(m_lock);

            int index = findSlotOrMakeSlot();
            if (index == -1)
                return -1;

            m_elements[index] = element(ptr);

            return index;
        }

        int addRef(T* ptr, sp_shared* counter) {
            ScopeSpinlock scopeLock(m_lock);

            int index = findSlotOrMakeSlot();
            if (index == -1)
                return -1;

            m_elements[index] = element(ptr, counter);

            return index;
        }

        void remove(int index) {
            ScopeSpinlock scopeLock(m_lock);

            if (index < 0 || index >= m_element_count)
                return;

            m_elements[index].die();
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

        Spinlock m_lock;

        int      m_element_count = 0;
        element* m_elements      = nullptr;

        int findSlotOrMakeSlot() {
            for (int i = 0; i < m_element_count; i++) {
                if (!m_elements[i].ptr || !m_elements[i].shared ||
                    m_elements[i].shared->ref_count() == 0) {
                    // printk("!present slot: %i\n", i);
                    return i;
                }
            }

            m_elements =
                (element*) Heap::realloc(m_elements, (m_element_count + 1) * sizeof(element));
            m_element_count++;

            return m_element_count - 1;
        }
    };
}