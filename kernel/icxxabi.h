#pragma once

#include "aex/utility.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#define ATEXIT_MAX_FUNCS 128

struct API atexit_func_entry {
    void (*destructor_func)(void*);
    void* obj_ptr;
    void* dso_handle;
};

API int  __cxa_atexit(void (*f)(void*), void* objptr, void* dso);
API void __cxa_finalize(void* f);

API extern void* __dso_handle;
API void         __cxa_pure_virtual() {}

#ifdef __cplusplus
}
#endif