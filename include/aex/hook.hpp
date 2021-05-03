#pragma once

#include "aex/errno.hpp"
#include "aex/mem/vector.hpp"
#include "aex/mutex.hpp"

namespace AEX {
    template <typename Func>
    class Hook {
        public:
        void subscribe(Func func) {
            SCOPE(m_mutex);
            m_funcs.push(func);
        }

        void unsubscribe(Func func) {
            SCOPE(m_mutex);
            for (int i = 0; i < m_funcs.count(); i++)
                if (m_funcs[i] == func) {
                    m_funcs.erase(i);
                    break;
                }
        }

        template <typename... Args>
        void invoke(Args... args) {
            SCOPE(m_mutex);

            for (int i = 0; i < m_funcs.count(); i++)
                m_funcs[i](args...);
        }

        private:
        Mem::Vector<Func> m_funcs;
        Mutex             m_mutex;
    };
}