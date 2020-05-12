#pragma once

namespace AEX::Dev::Tree {
    class Device;

    class Interface {
      public:
        virtual ~Interface();

        virtual bool bind(Device* device);
    };
}