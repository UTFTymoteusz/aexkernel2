#pragma once

#include "aex/mem/vector.hpp"
#include "aex/types.hpp"

#include "types.hpp"

namespace AEX::FS {
    class Chain {
        public:
        /**
         * Returns the cluster id at the specified index.
         * @returns Cluster id at the specified index.
         */
        cluster_t at(int index) {
            return m_clusters[index];
        }

        /**
         * Returns the total cluster count.
         * @returns The total count of clusters.
         */
        size_t count() {
            return m_clusters.count();
        }

        /**
         * Clears out the entire chain and sets the first cluster id.
         */
        void first(cluster_t first) {
            m_clusters.clear();
            m_clusters.push(first);
        }

        /**
         * Returns the first cluster id in the chain.
         * @returns The first cluster_id.
         */
        cluster_t first() {
            return m_clusters[0];
        }

        /**
         * Returns the last cluster id in the chain.
         * @returns The last cluster_id.
         */
        cluster_t last() {
            return m_clusters[count() - 1];
        }

        /**
         * Pushes a cluster id to the back of the chain.
         */
        void push(cluster_t cluster) {
            m_clusters.push(cluster);
        }

        /**
         * Erases the last entry.
         * @returns The erased cluster id.
         */
        cluster_t erase() {
            cluster_t clust = m_clusters[count() - 1];
            m_clusters.erase(count() - 1);
            return clust;
        }

        private:
        Mem::Vector<cluster_t> m_clusters;

        friend class FATControlBlock;
    };
}