#include "aex/proc/guards.hpp"

#include "aex/proc/thread.hpp"

namespace AEX::Proc {
    CriticalGuard::Scope::Scope(Thread* thread) {
        m_thread = thread;
        m_thread->addCritical();
    }

    CriticalGuard::Scope::~Scope() {
        m_thread->subCritical();
    }

    SignabilityGuard::Scope::Scope(Thread* thread) {
        m_thread = thread;
        m_thread->addSignability();
    }

    SignabilityGuard::Scope::~Scope() {
        m_thread->subSignability();
    }

    BusyGuard::Scope::Scope(Thread* thread) {
        m_thread = thread;
        m_thread->addBusy();
    }

    BusyGuard::Scope::~Scope() {
        m_thread->subBusy();
    }
}