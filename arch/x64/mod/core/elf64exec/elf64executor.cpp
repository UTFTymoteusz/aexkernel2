#include "elf64executor.hpp"

#include "aex/elf.hpp"
#include "aex/fs.hpp"

#include <stddef.h>
#include <stdint.h>

using namespace AEX;

error_t Elf64Executor::exec(Proc::Process* process, AEX::Proc::Thread* initiator, const char* path,
                            char* const argv[], char* const envp[]) {
    printk("elf64exec: Got a request for %s\n", path);

    auto file_try = FS::File::open(path, FS::O_RD);
    if (!file_try)
        return file_try.error_code;

    auto    file = file_try.value;
    int64_t size = file->seek(0, FS::File::SEEK_END).value;

    auto mmap_try =
        Mem::mmap(Proc::Process::kernel(), nullptr, size, Mem::PROT_READ, Mem::MAP_NONE, file, 0);
    file->close();

    if (!mmap_try)
        return mmap_try;

    void* addr = mmap_try.value;
    auto  elf  = ELF(addr);

    if (!elf.isValid(ELF::BIT_64, ELF::EN_LITTLE, ELF::ISA_AMD64))
        return ENOEXEC;

    // Temporary workaround to make the process not die when the last thread exits.
    Mem::atomic_add(&process->thread_counter, 1);

    abortall(process, initiator);

    for (int i = 0; i < elf.program_headers.count(); i++) {
        auto program_header = elf.program_headers[i];

        if (program_header.type != ELF::PH_TLS)
            continue;

        process->tls_size = program_header.memory_size;
    }

    for (int i = 0; i < elf.section_headers.count(); i++) {
        auto section_header = elf.section_headers[i];

        if (!(section_header.flags & ELF::SC_ALLOC) || section_header.size == 0)
            continue;

        size_t start = (size_t) section_header.address;
        size_t end   = start + section_header.size;

        size_t fptr = section_header.file_offset;

        for (size_t ptr = start; ptr < end;) {
            size_t chk_size = min(Sys::CPU::PAGE_SIZE - (ptr & 0x0FFF), end - ptr);

            Mem::phys_addr paddr = process->pagemap->paddrof((void*) ptr);
            if (!paddr) {
                void* vaddr = process->pagemap->alloc(
                    chk_size, PAGE_USER | PAGE_WRITE | PAGE_EXEC | PAGE_FIXED, (void*) ptr);
                paddr = process->pagemap->paddrof(vaddr);
            }

            if (section_header.type != ELF::sc_type_t::SC_NO_DATA) {
                void* kaddr = Mem::kernel_pagemap->map(chk_size, paddr, PAGE_WRITE);
                memcpy(kaddr, (uint8_t*) addr + fptr, chk_size);
                Mem::kernel_pagemap->free(kaddr, chk_size);
            }

            ptr += chk_size;
            fptr += chk_size;
        }
    }

    Mem::munmap(addr, size);

    int    argc_usr;
    char** argv_usr;

    // I need to make it cleanup the old argv after an exec
    setup_argv(process, argv, argc_usr, argv_usr);
    abortall(process);

    auto thread_try = Proc::Thread::create(process->pid, elf.entry, Proc::Thread::USER_STACK_SIZE,
                                           process->pagemap, true);
    if (!thread_try)
        kpanic("elf64executor is bork");

    thread_try.value->setArguments(argc_usr, argv_usr);

    // Temporary workaround to make the process not die when the last thread exits.
    Mem::atomic_sub(&process->thread_counter, 1);

    return ENONE;
}

void Elf64Executor::abortall(Proc::Process* process, Proc::Thread* except) {
    process->threads_lock.acquire();

    for (int i = 0; i < process->threads.count(); i++) {
        if (!process->threads.present(i))
            continue;

        if (process->threads[i] == except)
            continue;

        process->threads[i]->detach();
        process->threads[i]->abort();
    }

    process->threads_lock.release();

    while (true) {
        bool breakout = true;

        process->threads_lock.acquire();

        for (int i = 0; i < process->threads.count(); i++) {
            if (!process->threads.present(i))
                continue;

            if (process->threads[i] == except)
                continue;

            breakout = false;
            break;
        }

        process->threads_lock.release();

        if (breakout)
            break;

        Proc::Thread::sleep(25);
    }
}

void Elf64Executor::setup_argv(Proc::Process* process, char* const argv[], int& argc_out,
                               char**& argv_out) {
    int argc = 0;
    while (argc < 32 && argv[argc])
        argc++;

    int arg_size      = int_ceil<int>((argc + 1) * sizeof(char*), 16);
    int string_offset = arg_size;

    for (int i = 0; i < argc; i++)
        arg_size += int_ceil(strlen(argv[i]) + 1, 16);

    void* arg_usr = process->pagemap->alloc(arg_size, PAGE_USER);
    void* arg_krn =
        Mem::kernel_pagemap->map(arg_size, process->pagemap->paddrof(arg_usr), PAGE_WRITE);

    char** argv_vectors = (char**) arg_krn;

    for (int i = 0; i < argc; i++) {
        int len = strlen(argv[i]) + 1;
        memcpy((char*) ((size_t) arg_krn + string_offset), argv[i], len);

        argv_vectors[i] = (char*) ((size_t) arg_usr + string_offset);
        string_offset += int_ceil(len, 16);
    }

    Mem::kernel_pagemap->free(arg_krn, arg_size);

    argc_out = argc;
    argv_out = (char**) arg_usr;
}