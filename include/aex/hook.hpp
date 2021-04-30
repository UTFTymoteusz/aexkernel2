#pragma once

#include "aex/errno.hpp"
#include "aex/mem/vector.hpp"
#include "aex/mutex.hpp"

namespace AEX {
    template <typename Func>
    class Hook {
        public:
        void subscribe(Func func) {
            auto scope = m_mutex.scope();

            m_funcs.push(func);
        }

        void unsubscribe(Func) {
            auto scope = m_mutex.scope();
        }

        template <typename... Args>
        void invoke(Args... args) {
            auto scope = m_mutex.scope();

            for (int i = 0; i < m_funcs.count(); i++)
                m_funcs[i](args...);
        }

        private:
        Mem::Vector<Func> m_funcs;
        Mutex             m_mutex;
    };
}