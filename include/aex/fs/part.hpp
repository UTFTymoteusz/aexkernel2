#pragma once

#include "aex/dev/blockhandle.hpp"
#include "aex/errno.hpp"

namespace AEX::FS {
    error_t read_partitions(Dev::BlockHandle handle, int limit = 16);
}