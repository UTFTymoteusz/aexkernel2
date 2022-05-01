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

        for (auto& thread : m_tiddies)
            if (thread == current) {
                is_on_queue = true;
                break;
            }

        if (!is_on_queue)
            m_tiddies.push(current);

        if (timeout == 0) {
            current->status = TS_BLOCKED;
        }
        else {
            current->status    = TS_SLEEPING;
            current->wakeup_at = Sys::Time::uptime() + (time_t) timeout * 1000000;
        }

        m_lock.release();

        if (!current->isCritical())
            Thread::yield();
    }

    int Event::raise() {
        m_lock.acquire();

        int total = m_tiddies.count();

        for (auto& thread : m_tiddies) {
            ASSERT(thread->status != TS_DEAD);
            thread->status = TS_RUNNABLE;
        }

        m_tiddies.clear();
        m_lock.release();

        return total;
    }

    int Event::defunct() {
        m_lock.acquire();
        m_defunct = true;

        int total = m_tiddies.count();

        for (auto& thread : m_tiddies) {
            ASSERT(thread->status != TS_DEAD);
            thread->status = TS_RUNNABLE;
        }

        m_tiddies.clear();
        m_lock.release();

        return total;
    }

    void Event::nevermind() {
        m_lock.acquire();

        if (m_defunct) {
            m_lock.release();
            return;
        }

        auto current = Thread::current();

        for (int i = 0; i < m_tiddies.count(); i++)
            if (m_tiddies[i] == current) {
                m_tiddies.erase(i);
                break;
            }

        m_lock.release();
    }

    void SimpleEvent::wait(int timeout) {
        m_lock.acquire();

        if (m_defunct) {
            m_lock.release();
            return;
        }

        auto current = Thread::current();

        // We need to make sure anybody isn't waiting already.
        ASSERT(!m_tiddie);

        m_tiddie = current;

        if (timeout == 0) {
            current->status = TS_BLOCKED;
        }
        else {
            current->status    = TS_SLEEPING;
            current->wakeup_at = Sys::Time::uptime() + (uint64_t) timeout * 1000000;
        }

        m_lock.release();

        if (!current->isCritical())
            Thread::yield();
    }

    void SimpleEvent::raise() {
        m_lock.acquire();

        if (m_tiddie)
            m_tiddie->status = TS_RUNNABLE;

        m_tiddie = nullptr;
        m_lock.release();
    }

    void SimpleEvent::defunct() {
        m_lock.acquire();

        m_defunct = true;

        if (m_tiddie)
            m_tiddie->status = TS_RUNNABLE;

        m_tiddie = nullptr;
        m_lock.release();
    }
}
