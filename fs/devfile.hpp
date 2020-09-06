#pragma once

#include "aex/dev.hpp"
#include "aex/mem.hpp"

namespace AEX::FS {
    class DevFile : public File {
        public:
        DevFile(Mem::SmartPointer<Dev::Device> dev) {
            m_dev = dev;
        }

        private:
        Mem::SmartPointer<Dev::Device> m_dev;
    };
}