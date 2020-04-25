#pragma once

#include <stddef.h>

namespace AEX::Heap {
    /*
     * Allocates the specified amount of bytes on the heap and returns the pointer to it. Returns
     * nullptr on failure.
     */
    void* malloc(size_t size);

    /*
     * Frees a previously-allocated region of memory.
     */
    void free(void* ptr);

    /*
     * Returns the size of a previously-allocated region of memory in bytes.
     */
    size_t msize(void* ptr);

    /*
     * Resizes a previously-allocated region of memory, or returns a new region if passed pointer is
     * nullptr.
     */
    void* realloc(void* ptr, size_t size);

    void init();
}