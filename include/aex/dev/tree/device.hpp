#pragma once

#include "aex/dev/tree/resource.hpp"
#include "aex/mem.hpp"
#include "aex/mem/vector.hpp"
#include "aex/optional.hpp"
#include "aex/spinlock.hpp"
#include "aex/string.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Dev::Tree {
    class Device {
        public:
        Device*                 parent = nullptr;
        Mem::SmartArray<Device> children;

        char name[32];

        void* driver_data;

        Device(const char* name, Device* parent);
        virtual ~Device();

        void               add(resource resource);
        optional<resource> getResource(int index);

        virtual void registerDevice(Device* device);

        private:
        Spinlock m_lock;

        Mem::Vector<resource> m_resources;
    };
}