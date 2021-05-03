#pragma once

#include "aex/fs.hpp"
#include "aex/mem/smartptr.hpp"
#include "aex/mutex.hpp"
#include "aex/optional.hpp"
#include "aex/types.hpp"
#include "aex/utility.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Dev {
    class CharDevice;

    class API CharHandle {
        public:
        void* device_data = nullptr;

        CharHandle();
        CharHandle(Mem::SmartPointer<CharDevice> chrhndl);
        ~CharHandle();

        optional<ssize_t>      read(void* ptr, size_t len);
        optional<ssize_t>      write(const void* ptr, size_t len);
        optional<int>          ioctl(int rq, uint64_t val);
        optional<Mem::Region*> mmap(Proc::Process* process, void*, size_t len, int flags,
                                    FS::File_SP file, FS::off_t offset);
        bool                   isatty();

        private:
        Mem::SmartPointer<CharDevice> m_dev;
    };

    typedef Mem::SmartPointer<CharHandle> CharHandle_SP;
}
