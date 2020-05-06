#pragma once

#include "aex/dev/device.hpp"

#include <stdint.h>

namespace AEX::Dev::SATA {
    class AHCI;

    class SATADevice : public Device {
      public:
        SATADevice(const char* name) : Device(name) {}

        AHCI* controller;
    };
}