#pragma once

#include "aex/errno.hpp"
#include "aex/fs.hpp"
#include "aex/mem.hpp"
#include "aex/optional.hpp"
#include "aex/proc/thread.hpp"

#include <stddef.h>
#include <stdint.h>

#define USR_ERRNO (AEX::Proc::Thread::current()->errno)

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

AEX::optional<size_t> u2k_memcpy(void* dst, const usr_void* src, size_t len);
AEX::optional<size_t> k2u_memcpy(usr_void* dst, const void* src, size_t len);

AEX::optional<int> usr_strlen(const usr_char* str);

template <typename T>
AEX::optional<T> usr_read(const usr_void* src) {
    return *((T*) src);
}

template <typename T>
AEX::optional<T> usr_read(const T* src) {
    return *src;
}

template <typename T>
AEX::optional<T> usr_write(const usr_void* dst, T val) {
    *((T*) dst) = val;
    return val;
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

bool copy_and_canonize(char buffer[AEX::FS::MAX_PATH_LEN], const usr_char* usr_path);

#define USR_ENSURE_R(cond, err) \
    ({                          \
        auto res = cond;        \
        if (!res) {             \
            USR_ERRNO = err;    \
            return -1;          \
        }                       \
        res;                    \
    })
#define USR_ENSURE_OPT(opt)        \
    ({                             \
        auto res = opt;            \
        if (!res) {                \
            USR_ERRNO = res.error; \
            return -1;             \
        }                          \
        res.value;                 \
    })
#define USR_ENSURE_ENONE(exp)    \
    ({                           \
        auto res = exp;          \
        if (res != AEX::ENONE) { \
            USR_ERRNO = res;     \
            -1;                  \
        }                        \
        0;                       \
    })
#define USR_ENSURE(cond) USR_ENSURE_R((cond), AEX::EINVAL)
#define USR_ENSURE_FL(flags, mask) USR_ENSURE_R(!(flags & ~mask), AEX::EINVAL)