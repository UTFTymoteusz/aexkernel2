#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define ATEXIT_MAX_FUNCS 128

struct atexit_func_entry {
    void (*destructor_func)(void*);
    void* obj_ptr;
    void* dso_handle;
};

int  __cxa_atexit(void (*f)(void*), void* objptr, void* dso);
void __cxa_finalize(void* f);

#ifdef __cplusplus
}
#endif