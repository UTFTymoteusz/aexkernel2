#pragma once

#include "aex/dev/device.hpp"

namespace AEX::Dev::PCI {
    void set_busmaster(Device* device, bool on);
}