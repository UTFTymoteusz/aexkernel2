#pragma once

namespace AEX::Mem {
    template <typename T>
    inline T atomic_read(T* ptr) {
        return __sync_fetch_and_add(ptr, 0);
    }

    template <typename T>
    inline void atomic_add(T* ptr, T val) {
        __sync_add_and_fetch(ptr, val);
    }

    template <typename T>
    inline void atomic_sub(T* ptr, T val) {
        __sync_sub_and_fetch(ptr, val);
    }
}