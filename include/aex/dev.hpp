#pragma once

#include "aex/dev/blockdevice.hpp"
#include "aex/dev/chardevice.hpp"
#include "aex/dev/device.hpp"
#include "aex/dev/input/code.hpp"
#include "aex/dev/input/event.hpp"
#include "aex/dev/input/handle.hpp"
#include "aex/dev/input/keymap.hpp"
#include "aex/dev/name.hpp"
#include "aex/dev/netdevice.hpp"
#include "aex/dev/tree/bus.hpp"
#include "aex/dev/tree/device.hpp"
#include "aex/dev/tree/driver.hpp"
#include "aex/dev/tree/interface.hpp"
#include "aex/dev/tree/resource.hpp"
#include "aex/dev/tty/ansi.hpp"
#include "aex/dev/tty/tty.hpp"
#include "aex/dev/tty/vtty.hpp"
#include "aex/dev/types.hpp"
#include "aex/mem/smartarray.hpp"
#include "aex/utility.hpp"

namespace AEX::Dev {
    API extern Mem::SmartArray<Device> devices;
}