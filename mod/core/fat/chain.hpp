#pragma once

#include "aex/mem/vector.hpp"
#include "aex/types.hpp"

namespace AEX::FS {
    class Chain {
        public:
        uint32_t at(int index) {
            return m_clusters[index];
        }

        int count() {
            return m_clusters.count();
        }

        void first(uint32_t first) {
            m_clusters.clear();
            m_clusters.push(first);
        }

        private:
        Mem::Vector<uint32_t> m_clusters;

        friend class FATControlBlock;
    };
}