#include "aex/ipc/event.hpp"

#include "aex/proc/proc.hpp"
#include "aex/proc/process.hpp"
#include "aex/proc/thread.hpp"
#include "aex/spinlock.hpp"


using namespace AEX::Proc;

namespace AEX::IPC {
    // Make the scheduler continually try to wake up a thread if it's aborted.
    void Event::wait() {
        _lock.acquire();

        _tiddies.pushBack(Thread::getCurrentThread()->getSmartPointer());

        Thread::getCurrentThread()->setStatus(Thread::status_t::BLOCKED);

        _lock.release();
        Thread::yield();
    }

    int Event::raise() {
        _lock.acquire();

        int total = _tiddies.count();
        for (int i = 0; i < total; i++) {
            _tiddies.at(i)->setStatus(Thread::status_t::RUNNABLE);
            _tiddies.erase(i);
        }

        _lock.release();

        return total;
    }
}
