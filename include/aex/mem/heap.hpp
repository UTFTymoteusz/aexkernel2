#pragma once

#include <stddef.h>
#include <stdint.h>

namespace AEX::Mem::Heap {
    extern uint64_t heap_allocated;
    extern uint64_t heap_free;

    /**
     * Allocates memory the heap and returns the pointer to it. Returns
     * nullptr on failure.
     * @param size Amount of bytes to allocate.
     * @returns Pointer to the allocated memory.
     **///
    [[nodiscard]] void* malloc(size_t size);

    /**
     * Frees a previously-allocated region of memory.
     * @param ptr The pointer to a previously-allocated region of memory
     **///
    void free(void* ptr);

    /**
     * Returns the size of a previously-allocated region of memory in bytes.
     * @param ptr The pointer to a previously-allocated region of memory
     * @returns Size of the memory region.
     **///
    size_t msize(void* ptr);

    /**
     * Returns the size of a previously-allocated region of memory in bytes including any descriptor
     * information.
     * @param ptr The pointer to a previously-allocated region of memory.
     * @returns Size of the memory region counting its descriptors.
     **///
    size_t msize_total(void* ptr);

    /**
     * Predicts the actual amount of memory that the specified allocation size would use;
     * @param len Allocation length.
     * @returns Size of the memory region counting its descriptors.
     **///
    size_t msize_total(size_t len);

    /**
     * Resizes a previously-allocated region of memory, or returns a new region if passed pointer is
     * nullptr.
     * @param ptr The pointer to a previously-allocated region of memory.
     * @param size Amount of bytes to resize to.
     * @returns Pointer to the reallocated memory.
     **///
    [[nodiscard]] void* realloc(void* ptr, size_t size);

    template <typename T>
    [[nodiscard]] T realloc(T ptr, size_t size) {
        return (T) realloc((void*) ptr, size);
    }
}