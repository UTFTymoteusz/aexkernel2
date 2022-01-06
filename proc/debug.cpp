#include "aex/config.hpp"

#include "proc/proc.hpp"
#include "sys/mcore.hpp"

namespace AEX::Proc {
    extern int     thread_list_size;
    extern Thread* thread_list_head;
    extern Thread* thread_list_tail;

    void debug_print_cpu_jobs() {
        for (int i = 0; i < Sys::MCore::cpu_count; i++) {
            auto cpu = Sys::MCore::CPUs[i];

            void*       addr  = (void*) cpu->current_thread->context->rip;
            int         delta = 0;
            const char* name  = Debug::addr2name(addr, delta) ?: "no idea";

            printk("cpu%i: PID %8i, TID %8i @ %p <%s+0x%x> (c%i, i%i)\n", i,
                   cpu->current_thread->parent->pid, cpu->currentThread(), addr, name, delta,
                   cpu->current_thread->getCritical(), cpu->in_interrupt);
        }
    }

    void debug_print_threads() {
        // auto scope  = sched_lock.scope();
        auto thread = thread_list_head;

        for (int i = 0; i < thread_list_size; i++) {
            auto name = Debug::addr2name(thread->original_entry) ?: "no idea";

            printk("  %p %6i %12s %18p entry <%s> %s %p %li [cr %i, sg %i] %i lk%i %s\n", thread,
                   thread->parent->pid, strstatus(thread->status), thread->context->rip, name,
                   thread->detached() ? "detached" : (thread->joiner() ? "joined by" : ""),
                   thread->joiner(), thread->interrupted(), thread->getCritical(),
                   thread->getSignability(), thread->sched_counter, thread->lock.isAcquired(),
                   thread->isBusy() ? "krl" : "usr");

            if (thread->parent->pid == 3)
                thread->lock.trace();

#if KPANIC_PROC_THREAD_TRACE
            Debug::stack_trace(0, (Debug::stack_frame*) thread->context->rsp);
            printk("\n");
#endif

            // const char* name = Debug::addr2name(thread->original_entry);
            // name             = name ?: "no idea";

            /*printk("%p, <%s>, %p-%p (0x%lx), %p-%p (0x%lx)\n", thread, name,
                   thread->kernel_stack, thread->kernel_stack + thread->kernel_stack_size,
                   thread->kernel_stack_size, thread->fault_stack,
                   thread->fault_stack + thread->fault_stack_size, thread->fault_stack_size);*/
            thread = thread->next;
        }
    }

    void debug_print_idles() {
        for (int i = 0; i < Sys::MCore::cpu_count; i++) {
            auto thread = idle_threads[i];

            printk("  %p (p: %p, n: %p) <%s> <%s> %s %p\n", thread, thread->prev, thread->next,
                   strstatus(thread->status), Debug::addr2name(thread->original_entry),
                   thread->detached() ? "detached" : (thread->joiner() ? "joined by" : ""),
                   thread->joiner());
        }
    }

    void debug_print_processes() {
        auto process = process_list_head;

        for (int i = 0; i < process_list_size; i++) {
            printk("  %p: %i. %s, [%i, %i] %12s, uid: %i-%i-%i  gid: %i-%i-%i, ni %i, sid %i  pgrp "
                   "%i %s\n",
                   process, process->pid, process->name, process->threads.realCount(),
                   process->thread_counter, strstatus(process->status), process->real_uid,
                   process->eff_uid, process->saved_uid, process->real_gid, process->eff_gid,
                   process->saved_gid, process->nice, process->session, process->group,
                   process->get_cwd());
            printk("  ");

            if (process->threads.count() > 0) {
                for (auto& thread_try : process->threads)
                    printk("0x%08x ", (size_t) thread_try.value.get() & 0xFFFFFFFF);
            }
            else {
                printk("*no threads*");
            }

            printk("\n");

            process = process->next;
        }
    }
}