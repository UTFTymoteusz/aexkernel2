#include "proc/proc.hpp"
#include "sys/mcore.hpp"

namespace AEX::Proc {
    extern int     thread_list_size;
    extern Thread* thread_list_head;
    extern Thread* thread_list_tail;

    char* debug_serialize_flags(char* buffer, int flags) {
        int ptr = 0;

        const char* bongs[] = {"run", "slp", "blk", "unk", "unk", "unk", "unk", "ded"};

        for (int i = 0; i < 8; i++) {
            if (!(flags & (1 << i)))
                continue;

            if (ptr != 0)
                buffer[ptr++] = '/';

            memcpy(buffer + ptr, bongs[i], 3);
            ptr += 3;
        }

        buffer[ptr] = '\0';

        return buffer;
    }

    void debug_print_cpu_jobs() {
        for (int i = 0; i < Sys::MCore::cpu_count; i++) {
            auto cpu = Sys::MCore::CPUs[i];

            void* addr = (void*) cpu->current_thread->context->rip;

            int         delta = 0;
            const char* name  = Debug::addr2name(addr, delta);

            printk("cpu%i: PID %8i, TID %8i @ 0x%p <%s+0x%x> (b%i, c%i, i%i)\n", i,
                   cpu->current_thread->parent->pid, cpu->unused, addr, name ?: "no idea", delta,
                   cpu->current_thread->getBusy(), cpu->current_thread->getCritical(),
                   cpu->in_interrupt);
        }
    }

    void debug_print_threads() {
        // auto scope  = sched_lock.scope();
        auto thread = thread_list_head;

        for (int i = 0; i < thread_list_size; i++) {
            char buffer[32];
            debug_serialize_flags(buffer, thread->status);

            /*printk("0x%p (p: 0x%p, n: 0x%p) <%s> <%s> %s 0x%p\n", thread, thread->prev,
                   thread->next, buffer, Debug::addr2name(thread->original_entry),
                   thread->detached() ? "detached" : (thread->joiner() ? "joined by" : ""),
                   thread->joiner());*/

            auto name = Debug::addr2name(thread->original_entry) ?: "no idea";

            printk("  0x%p %6i <%s> <%s> %s 0x%p %li [%i, %i] %i %s %s\n", thread,
                   thread->parent->pid, buffer, name,
                   thread->detached() ? "detached" : (thread->joiner() ? "joined by" : ""),
                   thread->joiner(), thread->interrupted(), thread->getCritical(),
                   thread->getBusy(), thread->sched_counter,
                   thread->context->rflags & 0x200 ? "interrupts" : "nointerrupts",
                   thread->faulting ? "faulting" : "not faulting");

            // Debug::stack_trace(0, (Debug::stack_frame*) thread->context->rsp);
            // printk("\n");

            // const char* name = Debug::addr2name(thread->original_entry);
            // name             = name ?: "no idea";

            /*printk("0x%p, <%s>, 0x%p-0x%p (0x%lx), 0x%p-0x%p (0x%lx)\n", thread, name,
                   thread->kernel_stack, thread->kernel_stack + thread->kernel_stack_size,
                   thread->kernel_stack_size, thread->fault_stack,
                   thread->fault_stack + thread->fault_stack_size, thread->fault_stack_size);*/
            thread = thread->next;
        }
    }

    void debug_print_idles() {
        for (int i = 0; i < Sys::MCore::cpu_count; i++) {
            auto thread = idle_threads[i];

            char buffer[32];
            debug_serialize_flags(buffer, thread->status);

            printk("  0x%p (p: 0x%p, n: 0x%p) <%s> <%s> %s 0x%p\n", thread, thread->prev,
                   thread->next, buffer, Debug::addr2name(thread->original_entry),
                   thread->detached() ? "detached" : (thread->joiner() ? "joined by" : ""),
                   thread->joiner());
        }
    }

    void debug_print_processes() {
        printk("head: 0x%p, Count: %i\n", process_list_head, process_list_size);
        auto process = process_list_head;

        for (int i = 0; i < process_list_size; i++) {
            char buffer[32];
            debug_serialize_flags(buffer, process->status);

            printk("%i. %s, [%i, %i] <%s>, uid: %i-%i-%i  gid: %i-%i-%i, ni %i\n", process->pid,
                   process->name, process->threads.realCount(), process->thread_counter, buffer,
                   process->real_uid, process->eff_uid, process->saved_uid, process->real_gid,
                   process->eff_gid, process->saved_gid, process->nice);

            process->threads_lock.acquire();

            for (int i = 0; i < process->threads.count(); i++)
                printk("0x%x ", (size_t) process->threads[i] & 0xFFFFFF);

            process->threads_lock.release();

            printk("\n");

            process = process->next;
        }

        printk("tail: 0x%p\n", thread_list_tail);
    }
}