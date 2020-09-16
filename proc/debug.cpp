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
                   cpu->current_thread->parent->pid, cpu->unused, addr, name ? name : "no idea",
                   delta, cpu->current_thread->getBusy(), cpu->current_thread->getCritical(),
                   cpu->in_interrupt);
        }
    }

    void debug_print_list() {
        printk("head: 0x%p, Count: %i\n", thread_list_head, thread_list_size);
        auto thread = thread_list_head;

        for (int i = 0; i < thread_list_size; i++) {
            char buffer[32];
            debug_serialize_flags(buffer, thread->status);

            printk("0x%p (p: 0x%p, n: 0x%p) <%s> <%s> %s 0x%p\n", thread, thread->prev,
                   thread->next, buffer, Debug::addr2name(thread->original_entry),
                   thread->detached() ? "detached" : (thread->joiner() ? "joined by" : ""),
                   thread->joiner());
            thread = thread->next;
        }

        printk("tail: 0x%p\n", thread_list_tail);
        printk("idles:\n");

        for (int i = 0; i < Sys::MCore::cpu_count; i++) {
            thread = idle_threads[i];

            char buffer[32];
            debug_serialize_flags(buffer, thread->status);

            printk("0x%p (p: 0x%p, n: 0x%p) <%s> <%s> %s 0x%p\n", thread, thread->prev,
                   thread->next, buffer, Debug::addr2name(thread->original_entry),
                   thread->detached() ? "detached" : (thread->joiner() ? "joined by" : ""),
                   thread->joiner());
        }
    }

    void debug_print_processes() {
        for (auto iterator = processes.getIterator(); auto process = iterator.next();) {
            printk("%i. %s, %i\n", process->pid, process->name, process->threads.realCount());

            process->lock.acquire();

            for (int i = 0; i < process->threads.count(); i++)
                printk("0x%x ", (size_t) process->threads[i] & 0xFFFFFF);

            process->lock.release();

            printk("\n");
        }
    }
}
