#pragma once

namespace AEX::Dev {
    class Device;

    class Driver {
      public:
        char name[32];

        Driver(const char* name);
        virtual ~Driver();

        virtual bool check(Device* device);
        virtual void bind(Device* device);
    };
}