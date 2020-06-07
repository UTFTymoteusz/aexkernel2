#include "aex/ipc/event.hpp"

#include "aex/proc/proc.hpp"
#include "aex/proc/process.hpp"
#include "aex/proc/thread.hpp"
#include "aex/spinlock.hpp"
#include "aex/sys/time.hpp"

using namespace AEX::Proc;

namespace AEX::IPC {
    void Event::wait(int timeout) {
        _lock.acquire();

        if (_defunct) {
            _lock.release();
            return;
        }

        auto current_sptr = Thread::getCurrentThread()->getSmartPointer();
        bool is_on_queue  = false;

        for (int i = 0; i < _tiddies.count(); i++)
            if (_tiddies[i].get() == current_sptr.get()) {
                is_on_queue = true;
                break;
            }

        if (!is_on_queue)
            _tiddies.pushBack(current_sptr);

        if (timeout == 0)
            current_sptr->setStatus(Thread::status_t::BLOCKED);
        else {
            current_sptr->setStatus(Thread::status_t::SLEEPING);
            current_sptr->wakeup_at = Sys::get_uptime() + (uint64_t) timeout * 1000000;
        }

        _lock.release();

        if (!current_sptr->isCritical())
            Thread::yield();
    }

    int Event::raise() {
        _lock.acquire();

        int total = _tiddies.count();
        for (int i = 0; i < total; i++)
            _tiddies.at(i)->setStatus(Thread::status_t::RUNNABLE);

        _tiddies.clear();
        _lock.release();

        return total;
    }

    int Event::defunct() {
        _lock.acquire();

        _defunct = true;

        int total = _tiddies.count();
        for (int i = 0; i < total; i++)
            _tiddies.at(i)->setStatus(Thread::status_t::RUNNABLE);

        _tiddies.clear();
        _lock.release();

        return total;
    }

    void SimpleEvent::wait(int timeout) {
        _lock.acquire();

        if (_defunct) {
            _lock.release();
            return;
        }

        auto current_sptr = Thread::getCurrentThread()->getSmartPointer();

        if (_tiddie.isValid())
            kpanic("simpleevent: Tried to wait while another boi was waiting already");

        _tiddie = current_sptr;

        if (timeout == 0)
            current_sptr->setStatus(Thread::status_t::BLOCKED);
        else {
            current_sptr->setStatus(Thread::status_t::SLEEPING);
            current_sptr->wakeup_at = Sys::get_uptime() + (uint64_t) timeout * 1000000;
        }

        _lock.release();

        if (!current_sptr->isCritical())
            Thread::yield();
    }

    void SimpleEvent::raise() {
        _lock.acquire();

        if (_tiddie.isValid())
            _tiddie->setStatus(Thread::status_t::RUNNABLE);

        _tiddie = Mem::SmartPointer<Proc::Thread>(nullptr, nullptr);
        _lock.release();
    }

    void SimpleEvent::defunct() {
        _lock.acquire();

        _defunct = true;

        if (_tiddie.isValid())
            _tiddie->setStatus(Thread::status_t::RUNNABLE);

        _tiddie = Mem::SmartPointer<Proc::Thread>(nullptr, nullptr);
        _lock.release();
    }
}
