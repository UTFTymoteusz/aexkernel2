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
                base_queue->_lock.acquire();

                for (int i = 0; i < base_queue->_queries.count(); i++) {
                    auto query = base_queue->_queries[i];
                    if (query != base_query)
                        continue;

                    base_queue->_queries.erase(i);

                    break;
                }

                base_queue->_lock.release();

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
                base_queue->_lock.acquire();
            }

            ~iterator() {
                base_queue->_lock.release();
            }

            query* next() {
                if (index >= base_queue->_queries.count())
                    return nullptr;

                index++;
                return base_queue->_queries[index - 1];
            }
        };

        promise startQuery(T* base, int timeout = default_timeout) {
            auto _query  = new query();
            _query->base = base;

            auto _promise = promise();

            _promise.base_query = _query;
            _promise.base_queue = this;

            _lock.acquire();

            _queries.pushBack(_query);

            _query->event.wait(timeout);
            _lock.release();

            if (!Proc::Thread::getCurrent()->isCritical())
                Proc::Thread::yield();

            return _promise;
        }

        iterator getIterator() {
            return iterator(this);
        }

        private:
        Spinlock                    _lock;
        Mem::Vector<query*, 16, 16> _queries;
    };
}
