#pragma once

#include "aex/fs/file.hpp"
#include "aex/types.hpp"

namespace AEX::FS {
    class DevFile : public File {
        public:
        DevFile(Mem::SmartPointer<Dev::Device> dev) {
            m_dev = dev;
        }

        static optional<File*> open(Mem::SmartPointer<Dev::Device> dev, int mode);

        private:
        Mem::SmartPointer<Dev::Device> m_dev;
    };
}