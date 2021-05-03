#pragma once

#include "aex/dev/tree/resource.hpp"
#include "aex/mem.hpp"
#include "aex/mem/vector.hpp"
#include "aex/optional.hpp"
#include "aex/spinlock.hpp"
#include "aex/string.hpp"
#include "aex/utility.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Dev::Tree {
    class API Device {
        public:
        char name[32];

        Device*                 parent = nullptr;
        Mem::SmartArray<Device> children;

        void* driver_data;

        Device(const char* name, Device* parent);
        virtual ~Device();

        void               add(resource resource);
        optional<resource> get(int index);

        virtual void registerDevice(Device* device);

        private:
        Spinlock m_lock;

        Mem::Vector<resource> m_resources;
    };
}