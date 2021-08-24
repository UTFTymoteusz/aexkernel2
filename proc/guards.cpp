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

    void CriticalGuard::acquire() {
        m_thread->addCritical();
    }

    void CriticalGuard::release() {
        m_thread->subCritical();
    }

    SignabilityGuard::Scope::Scope(Thread* thread) {
        m_thread = thread;
        m_thread->addSignability();
    }

    SignabilityGuard::Scope::~Scope() {
        m_thread->subSignability();
    }

    void SignabilityGuard::acquire() {
        m_thread->addSignability();
    }

    void SignabilityGuard::release() {
        m_thread->subSignability();
    }
}