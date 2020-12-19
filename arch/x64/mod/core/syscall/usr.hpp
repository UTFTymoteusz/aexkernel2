#pragma once

#include "aex/mem.hpp"
#include "aex/optional.hpp"

#include <stddef.h>
#include <stdint.h>

typedef char  usr_char;
typedef short usr_short;
typedef int   usr_int;
typedef long  usr_long;

typedef void usr_void;

typedef uint8_t  usr_uint8_t;
typedef uint16_t usr_uint16_t;
typedef uint32_t usr_uint32_t;
typedef uint64_t usr_uint64_t;

typedef int8_t  usr_int8_t;
typedef int16_t usr_int16_t;
typedef int32_t usr_int32_t;
typedef int64_t usr_int64_t;

typedef size_t usr_size_t;

AEX::optional<size_t> usr_memcpy(void* dst, const usr_void* src, size_t len);
AEX::optional<int>    usr_strlen(const usr_char* str);

template <typename T>
AEX::optional<T> usr_read(const usr_void* src) {
    return *((T*) src);
}

template <typename T>
class tmp_array {
    public:
    tmp_array() {}
    tmp_array(size_t count) {
        array = new T[count];
    }

    ~tmp_array() {
        if (array)
            delete array;

        array = nullptr;
    }

    const T& operator[](int index) {
        return array[index];
    }

    T* get() {
        return array;
    }

    void resize(size_t count) {
        array = AEX::Mem::Heap::realloc(array, count);
    }

    private:
    T* array = nullptr;
};