#include "aex/ipc/event.hpp"

#include "aex/proc/proc.hpp"
#include "aex/proc/process.hpp"
#include "aex/proc/thread.hpp"
#include "aex/spinlock.hpp"

using namespace AEX::Proc;

namespace AEX::IPC {
    void Event::wait() {
        _lock.acquire();

        if (_defunct) {
            _lock.release();
            return;
        }

        auto current_sptr = Thread::getCurrentThread()->getSmartPointer();

        for (int i = 0; i < _tiddies.count(); i++)
            if (_tiddies[i].get() == current_sptr.get())
                kpanic("Tried to Event::wait() whilst the thread is already in the queue.");

        _tiddies.pushBack(current_sptr);

        current_sptr->setStatus(Thread::status_t::BLOCKED);

        _lock.release();
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
}
