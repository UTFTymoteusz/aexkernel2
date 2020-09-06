#include "aex/ipc/event.hpp"

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

        auto current_sptr = Thread::getCurrent()->getSmartPointer();
        bool is_on_queue  = false;

        for (int i = 0; i < m_tiddies.count(); i++)
            if (m_tiddies[i].get() == current_sptr.get()) {
                is_on_queue = true;
                break;
            }

        if (!is_on_queue)
            m_tiddies.pushBack(current_sptr);

        if (timeout == 0)
            current_sptr->setStatus(TS_BLOCKED);
        else {
            current_sptr->setStatus(TS_SLEEPING);
            current_sptr->wakeup_at = Sys::Time::uptime() + (uint64_t) timeout * 1000000;
        }

        m_lock.release();

        if (!current_sptr->isCritical())
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

        auto current_sptr = Thread::getCurrent()->getSmartPointer();

        if (m_tiddie)
            kpanic("simpleevent: Tried to wait while another boi was waiting already");

        m_tiddie = current_sptr;

        if (timeout == 0)
            current_sptr->setStatus(TS_BLOCKED);
        else {
            current_sptr->setStatus(TS_SLEEPING);
            current_sptr->wakeup_at = Sys::Time::uptime() + (uint64_t) timeout * 1000000;
        }

        m_lock.release();

        if (!current_sptr->isCritical())
            Thread::yield();
    }

    void SimpleEvent::raise() {
        m_lock.acquire();

        if (m_tiddie)
            m_tiddie->setStatus(TS_RUNNABLE);

        m_tiddie = Mem::SmartPointer<Proc::Thread>(nullptr, nullptr);
        m_lock.release();
    }

    void SimpleEvent::defunct() {
        m_lock.acquire();

        m_defunct = true;

        if (m_tiddie)
            m_tiddie->setStatus(TS_RUNNABLE);

        m_tiddie = Mem::SmartPointer<Proc::Thread>(nullptr, nullptr);
        m_lock.release();
    }
}
