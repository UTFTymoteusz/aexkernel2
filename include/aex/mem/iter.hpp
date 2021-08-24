#pragma once

namespace AEX::Mem {
    template <typename T>
    class Iter {
        public:
        Iter(T* begin, T* end) : m_begin(begin), m_end(end) {}

        T* begin() {
            return m_begin;
        }

        T* end() {
            return m_end;
        }

        private:
        T* m_begin;
        T* m_end;
    };
}