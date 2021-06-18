#include "cdev.hpp"

#include "aex/dev/tty/tty.hpp"
#include "aex/mem/align.hpp"
#include "aex/proc/process.hpp"
#include "aex/types.hpp"

#include "ioctl.hpp"

namespace AEX::Dev {
    TTYChar::TTYChar(int index, const char* name) : CharDevice(name) {
        m_index = index;
        m_tty   = TTY::TTYs[index];
    }

    error_t TTYChar::open(CharHandle* handle, int) {
        SCOPE(m_mutex);

        // m_stack.push(handle); // i need an insert() method
        m_current = handle;

        return ENONE;
    }

    error_t TTYChar::close(CharHandle* handle) {
        SCOPE(m_mutex);

        if (m_closed)
            return EINVAL;

        m_closed = true;

        for (int i = 0; i < m_stack.count(); i++) {
            if (m_stack[i] != handle)
                continue;

            m_stack.erase(i);
            break;
        }

        m_current = m_stack.count() ? m_stack[0] : nullptr;

        return ENONE;
    }

    optional<ssize_t> TTYChar::read(CharHandle*, void* ptr, size_t len) {
        auto cptr = (char*) ptr;

        for (size_t i = 0; i < len; i++) {
            int v = m_tty->read();
            if (v == -1)
                return i;

            // TODO: Figure out if this is correct at all
            if (v == -2) {
                IPC::siginfo_t info = {};
                info.si_signo       = IPC::SIGTSTP;

                Proc::Thread::current()->signal(info);
            }

            cptr[i] = v;
        }

        return len;
    }

    optional<ssize_t> TTYChar::write(CharHandle*, const void* ptr, size_t len) {
        auto cptr = (char*) ptr;

        for (size_t i = 0; i < len; i++)
            m_tty->write(cptr[i]);

        return len;
    }

    optional<int> TTYChar::ioctl(CharHandle*, int rq, uint64_t val) {
        switch (rq) {
        case VTMODE:
            switch (val) {
            case VT_TEXT:
                printk("tty switched to text\n");
                return m_tty->text() ? 0 : -1;
            case VT_GRAPHICAL:
                printk("tty switched to graphical\n");
                return m_tty->graphics() ? 0 : -1;
            default:
                return EINVAL;
            }
            break;
        case VTWIDTH:
            return m_tty->info().width;
        case VTHEIGHT:
            return m_tty->info().height;
        case VTGRWIDTH:
            return m_tty->info().gr_width;
        case VTGRHEIGHT:
            return m_tty->info().gr_height;
        case VTGRDEPTH:
            return m_tty->info().gr_depth;
        case VTGRBYTES:
            return m_tty->info().gr_bytes;
        case VTTCGETPGRP:
            return m_tty->tcgetpgrp();
        case VTTCSETPGRP:
            m_tty->tcsetpgrp(val);
            return 0;
        default:
            return EINVAL;
        }
    }

    optional<Mem::Region*> TTYChar::mmap(Proc::Process* process, void*, size_t len, int flags,
                                         FS::File_SP file, FS::off_t offset) {
        if (!Mem::pagealigned(offset))
            return EINVAL;

        auto info = m_tty->info();

        len = Mem::pageceil(len);
        if (len + offset > Mem::pageceil<size_t>(info.gr_bytes))
            return ERANGE;

        auto addr =
            process->pagemap->map(len, Mem::kernel_pagemap->paddrof(m_tty->output()), flags);

        return new Mem::Region(process->pagemap, addr, len, file, offset);
    }

    bool TTYChar::isatty() {
        return true;
    }
}