#pragma once

#include "aex/mem/lazyvector.hpp"

namespace AEX::Mem {
    template <typename T>
    class Cache {
        public:
        optional<T> get(int id) {
            if (!m_vector.present(id))
                return {};

            return m_vector.at(id);
        }

        void set(int id, T val) {
            m_vector.set(id, val);
        }

        int push(T val) {
            return m_vector.push(val);
        }

        void erase(int id) {
            m_vector.erase(id);
        }

        private:
        LazyVector<T> m_vector;
    };
}