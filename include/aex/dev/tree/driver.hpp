#pragma once

#include "aex/utility.hpp"

namespace AEX::Dev::Tree {
    class Device;

    class API Driver {
        public:
        char name[32];

        Driver(const char* name);
        virtual ~Driver();

        virtual bool check(Device* device) = 0;
        virtual void bind(Device* device)  = 0;
    };
}