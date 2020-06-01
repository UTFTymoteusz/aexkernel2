#pragma once

#include "aex/dev/device.hpp"
#include "aex/errno.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Dev {
    class Net : public Device {
      public:
        Net(const char* name);

        virtual ~Net();

        error_t send(const void* buffer, size_t len);

      private:
    };
}