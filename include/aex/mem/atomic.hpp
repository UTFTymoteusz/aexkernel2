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
    inline T atomic_fetch_add(T* ptr, T val) {
        return __sync_fetch_and_add(ptr, val);
    }

    template <typename T>
    inline T atomic_add_fetch(T* ptr, T val) {
        return __sync_add_and_fetch(ptr, val);
    }

    template <typename T>
    inline void atomic_sub(T* ptr, T val) {
        __sync_sub_and_fetch(ptr, val);
    }

    template <typename T>
    inline T atomic_fetch_sub(T* ptr, T val) {
        return __sync_fetch_and_sub(ptr, val);
    }

    template <typename T>
    inline T atomic_sub_fetch(T* ptr, T val) {
        return __sync_sub_and_fetch(ptr, val);
    }

    template <typename T>
    inline T atomic_compare_and_swap(T* ptr, T old, T m_new) {
        return __sync_val_compare_and_swap(ptr, old, m_new);
    }
}