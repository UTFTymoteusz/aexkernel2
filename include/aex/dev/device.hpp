#pragma once

#include "aex/optional.hpp"
#include "aex/rcparray.hpp"
#include "aex/spinlock.hpp"
#include "aex/string.hpp"
#include "aex/vector.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Dev {
    class Device {
      public:
        struct resource {
            enum type_t : uint8_t {
                MEMORY = 0,
                IO     = 1,
            };

            type_t type;

            union {
                size_t start;
                size_t value;
            };
            size_t end;

            resource() {}

            resource(type_t type, size_t value) {
                this->type  = type;
                this->value = value;
                this->end   = value;
            }

            resource(type_t type, size_t start, size_t end) {
                this->type  = type;
                this->start = start;
                this->end   = end;
            }
        };

        Device*          parent;
        RCPArray<Device> children;

        char name[32];

        void* driver_data;

        Device(const char* name);
        virtual ~Device();

        bool setInterface(const char* name);

        void                       addResource(resource resource);
        optional<Device::resource> getResource(int index);

        virtual void registerDevice(Device* device);

      private:
        Spinlock _lock;

        Vector<resource> _resources;
    };
}