#pragma once

#include "aex/fs/path.hpp"

namespace AEX::FS {
    class Mount {
      public:
        char path[Path::MAX_PATH_LEN];

        virtual ~Mount();

        virtual void unmount();

      private:
    };
}