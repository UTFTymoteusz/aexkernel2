#pragma once

#include <stddef.h>
#include <stdint.h>

inline void* operator new(size_t, void* ptr) {
    return ptr;
}

inline void* operator new[](size_t, void* ptr) {
    return ptr;
}

inline void operator delete(void*, void*) {}

inline void operator delete[](void*, void*) {}