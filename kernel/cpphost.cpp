#include "kernel/printk.hpp"
#include "mem/heap.hpp"

#include <stddef.h>

typedef size_t sizetype;

void* operator new(size_t size) { return AEX::Heap::malloc(size); }

void* operator new[](size_t size) { return AEX::Heap::malloc(size); }

void operator delete(void* ptr) { return AEX::Heap::free(ptr); }

void operator delete[](void* ptr) { return AEX::Heap::free(ptr); }

void operator delete(void* ptr, size_t size) { return AEX::Heap::free(ptr); }

void operator delete[](void* ptr, size_t size) { return AEX::Heap::free(ptr); }