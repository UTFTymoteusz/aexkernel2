#pragma once

#include "aex/fs/types.hpp"
#include "aex/proc/types.hpp"
#include "aex/sec/types.hpp"
#include "aex/sys/time/types.hpp"
#include "aex/utility.hpp"

#include <stddef.h>
#include <stdint.h>

#if BIT64
typedef int64_t  ssize_t;
typedef uint64_t size_t;
#else
typedef int32_t  ssize_t;
typedef uint32_t size_t;
#endif

typedef int id_t;