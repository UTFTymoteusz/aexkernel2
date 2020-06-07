#include "aex/mem/heap.hpp"

#include "kernel/icxxabi.h"

void* __dso_handle = 0;

size_t            __atexit_func_count = 0;
atexit_func_entry __atexit_funcs[ATEXIT_MAX_FUNCS];

void* operator new(size_t size) {
    return AEX::Heap::malloc(size);
}

void* operator new[](size_t size) {
    return AEX::Heap::malloc(size);
}

void operator delete(void* ptr) {
    return AEX::Heap::free(ptr);
}

void operator delete[](void* ptr) {
    return AEX::Heap::free(ptr);
}

void operator delete(void* ptr, size_t) {
    return AEX::Heap::free(ptr);
}

void operator delete[](void* ptr, size_t) {
    return AEX::Heap::free(ptr);
}

int __cxa_atexit(void (*f)(void*), void* objptr, void* dso) {
    if (__atexit_func_count >= ATEXIT_MAX_FUNCS)
        return -1;

    __atexit_funcs[__atexit_func_count].destructor_func = f;
    __atexit_funcs[__atexit_func_count].obj_ptr         = objptr;
    __atexit_funcs[__atexit_func_count].dso_handle      = dso;
    __atexit_func_count++;

    return 0;
};
