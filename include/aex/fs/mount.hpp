#pragma once

#include "aex/fs/path.hpp"

namespace AEX::FS {
    class Mount {
      public:
        char path[Path::MAX_LEN];

        virtual void unmount();

      private:
    };
}