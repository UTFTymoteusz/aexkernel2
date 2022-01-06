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

bool copy_and_canonize(char buffer[AEX::FS::PATH_MAX], const usr_char* usr_path);

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