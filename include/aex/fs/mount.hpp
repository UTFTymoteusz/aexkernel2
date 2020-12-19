#pragma once

#include "aex/errno.hpp"
#include "aex/fs/controlblock.hpp"
#include "aex/fs/path.hpp"
#include "aex/fs/types.hpp"
#include "aex/mem/smartptr.hpp"
#include "aex/optional.hpp"

namespace AEX::FS {
    class Mount {
        public:
        char path[MAX_PATH_LEN];

        virtual ~Mount();

        virtual void unmount();

        Mem::SmartPointer<ControlBlock> control_block;
    };
}