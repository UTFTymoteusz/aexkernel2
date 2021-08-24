#pragma once

#include "aex/types.hpp"

namespace AEX::IPC {
    struct sigset_t {
        uint32_t quads[4];

        void add(int sig) {
            quads[sig / 32] |= (1 << (sig % 32));
        }

        void del(int sig) {
            quads[sig / 32] &= ~(1 << (sig % 32));
        }

        bool member(int sig) {
            return quads[sig / 32] & (1 << (sig % 32));
        }

        sigset_t& block(const sigset_t& that) {
            for (int i = 0; i < 4; i++)
                this->quads[i] |= that.quads[i];

            return *this;
        }

        sigset_t& unblock(const sigset_t& that) {
            for (int i = 0; i < 4; i++)
                this->quads[i] &= ~that.quads[i];

            return *this;
        }

        sigset_t& set(const sigset_t& that) {
            for (int i = 0; i < 4; i++)
                this->quads[i] = that.quads[i];

            return *this;
        }
    };

    struct sigaction;

    // An architecture-specific struct containing the state of registers required to properly return
    // from a syscall.
    struct syscall_registers;

    // An architecture-specific struct containing the state of registers upon an entry into an
    // interrupt.
    struct interrupt_registers;

    // A combination of syscall_registers and interrupt_registers to make life easier;
    struct state_combo;
}