#include "aex/ipc/event.hpp"

#include "aex/assert.hpp"
#include "aex/proc.hpp"
#include "aex/spinlock.hpp"
#include "aex/sys/time.hpp"

using namespace AEX::Proc;

namespace AEX::IPC {
    void Event::wait(int timeout) {
        m_lock.acquire();

        if (m_defunct) {
            m_lock.release();
            return;
        }

        auto current     = Thread::current();
        bool is_on_queue = false;

        for (int i = 0; i < m_tiddies.count(); i++)
            if (m_tiddies[i] == current) {
                is_on_queue = true;
                break;
            }

        if (!is_on_queue)
            m_tiddies.push(current);

        if (timeout == 0)
            current->setStatus(TS_BLOCKED);
        else {
            current->setStatus(TS_SLEEPING);
            current->wakeup_at = Sys::Time::uptime() + (uint64_t) timeout * 1000000;
        }

        m_lock.release();

        if (!current->isCritical())
            Thread::yield();
    }

    int Event::raise() {
        m_lock.acquire();

        int total = m_tiddies.count();
        for (int i = 0; i < total; i++)
            m_tiddies.at(i)->setStatus(TS_RUNNABLE);

        m_tiddies.clear();
        m_lock.release();

        return total;
    }

    int Event::defunct() {
        m_lock.acquire();

        m_defunct = true;

        int total = m_tiddies.count();
        for (int i = 0; i < total; i++)
            m_tiddies.at(i)->setStatus(TS_RUNNABLE);

        m_tiddies.clear();
        m_lock.release();

        return total;
    }

    void SimpleEvent::wait(int timeout) {
        m_lock.acquire();

        if (m_defunct) {
            m_lock.release();
            return;
        }

        auto current = Thread::current();

        // We need to make sure anybody isn't waiting already.
        AEX_ASSERT(!m_tiddie);

        m_tiddie = current;

        if (timeout == 0)
            current->setStatus(TS_BLOCKED);
        else {
            current->setStatus(TS_SLEEPING);
            current->wakeup_at = Sys::Time::uptime() + (uint64_t) timeout * 1000000;
        }

        m_lock.release();

        if (!current->isCritical())
            Thread::yield();
    }

    void SimpleEvent::raise() {
        m_lock.acquire();

        if (m_tiddie)
            m_tiddie->setStatus(TS_RUNNABLE);

        m_tiddie = nullptr;
        m_lock.release();
    }

    void SimpleEvent::defunct() {
        m_lock.acquire();

        m_defunct = true;

        if (m_tiddie)
            m_tiddie->setStatus(TS_RUNNABLE);

        m_tiddie = nullptr;
        m_lock.release();
    }
}
