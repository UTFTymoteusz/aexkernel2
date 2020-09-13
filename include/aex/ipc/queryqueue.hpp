#pragma once

#include "aex/ipc/event.hpp"
#include "aex/mem/vector.hpp"
#include "aex/optional.hpp"
#include "aex/proc.hpp"
#include "aex/spinlock.hpp"

namespace AEX::IPC {
    template <typename T, int default_timeout>
    class QueryQueue {
        public:
        struct query {
            T*          base;
            SimpleEvent event;

            bool success = false;

            void notify_of_success() {
                success = true;
                event.raise();
            }

            void notify_of_failure() {
                success = false;
                event.raise();
            }

            T& operator*() {
                return *base;
            }

            T* operator->() {
                return base;
            }
        };

        struct promise {
            query*      base_query;
            QueryQueue* base_queue;

            ~promise() {
                base_queue->m_lock.acquire();

                for (int i = 0; i < base_queue->m_queries.count(); i++) {
                    auto query = base_queue->m_queries[i];
                    if (query != base_query)
                        continue;

                    base_queue->m_queries.erase(i);

                    break;
                }

                base_queue->m_lock.release();

                delete base_query;
            }

            query& operator*() {
                return *base_query;
            }

            query* operator->() {
                return base_query;
            }
        };

        struct iterator {
            int index = 0;

            QueryQueue* base_queue;

            iterator(QueryQueue* queue) {
                base_queue = queue;
                base_queue->m_lock.acquire();
            }

            ~iterator() {
                base_queue->m_lock.release();
            }

            query* next() {
                if (index >= base_queue->m_queries.count())
                    return nullptr;

                index++;
                return base_queue->m_queries[index - 1];
            }
        };

        promise startQuery(T* base, int timeout = default_timeout) {
            auto m_query  = new query();
            m_query->base = base;

            auto m_promise = promise();

            m_promise.base_query = m_query;
            m_promise.base_queue = this;

            m_lock.acquire();

            m_queries.pushBack(m_query);

            m_query->event.wait(timeout);
            m_lock.release();

            if (!Proc::Thread::current()->isCritical())
                Proc::Thread::yield();

            return m_promise;
        }

        iterator getIterator() {
            return iterator(this);
        }

        private:
        Spinlock                    m_lock;
        Mem::Vector<query*, 16, 16> m_queries;
    };
}
