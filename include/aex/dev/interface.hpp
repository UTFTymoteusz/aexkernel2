#pragma once

namespace AEX::Dev {
    class Device;

    class Interface {
      public:
        char name[24];

        Interface(const char* name);
        virtual ~Interface();

        virtual bool bind(Device* device);

      private:
    };
}