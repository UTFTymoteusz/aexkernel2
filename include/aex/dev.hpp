#pragma once

#include "aex/dev/blockdevice.hpp"
#include "aex/dev/device.hpp"
#include "aex/dev/name.hpp"
#include "aex/dev/netdevice.hpp"
#include "aex/dev/types.hpp"
#include "aex/mem/smartarray.hpp"

namespace AEX::Dev {
    extern Mem::SmartArray<Device> devices;
}