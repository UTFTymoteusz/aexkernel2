#pragma once

#include "aex/mem/vector.hpp"
#include "aex/types.hpp"

#include "types.hpp"

namespace AEX::FS {
    class Chain {
        public:
        cluster_t at(int index) {
            return m_clusters[index];
        }

        size_t count() {
            return m_clusters.count();
        }

        void first(cluster_t first) {
            m_clusters.clear();
            m_clusters.push(first);
        }

        private:
        Mem::Vector<cluster_t> m_clusters;

        friend class FATControlBlock;
    };
}