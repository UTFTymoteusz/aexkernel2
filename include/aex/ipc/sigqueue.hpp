#pragma once

#include "aex/assert.hpp"
#include "aex/ipc/types.hpp"
#include "aex/mem/iter.hpp"
#include "aex/mem/vector.hpp"
#include "aex/optional.hpp"

namespace AEX::IPC {
    class SigQueue {
        public:
        sigset_t mask;

        siginfo_t& peek(int index) {
            ASSERT(index >= 0 && index < m_queue.count());
            return m_queue[index];
        }

        int push(siginfo_t& info) {
            for (int i = 0; i < m_queue.count(); i++) {
                if (info.si_signo <= SIGSYS && m_queue[i].si_signo == info.si_signo)
                    return -1;

                if (m_queue[i].si_signo > info.si_signo) {
                    m_queue.insert(i, info);
                    return i;
                }
            }

            if (!mask.member(info.si_signo))
                m_countUnmasked++;

            return m_queue.push(info);
        }

        siginfo_t erase(int index) {
            auto info = m_queue[index];
            m_queue.erase(index);

            if (!mask.member(info.si_signo))
                m_countUnmasked--;

            return info;
        }

        void recalculate() {
            m_countUnmasked = 0;

            for (int i = 0; i < m_queue.count(); i++)
                if (!mask.member(m_queue[i].si_signo))
                    m_countUnmasked++;
        }

        sigset_t pending() {
            sigset_t set = {};

            for (int i = 0; i < m_queue.count(); i++)
                set.add(m_queue[i].si_signo);

            return set;
        }

        int count() {
            return m_queue.count();
        }

        int countUnmasked() {
            return m_countUnmasked;
        }

        private:
        Mem::Vector<siginfo_t> m_queue;
        int                    m_countUnmasked;
    };
}