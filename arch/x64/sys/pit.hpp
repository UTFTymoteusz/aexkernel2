#pragma once

namespace AEX::Sys {
    class PIT {
      public:
        static void setHz(int hz);
        static void setInterval(double ms);

        static void interruptIn(double ms);
    };
}