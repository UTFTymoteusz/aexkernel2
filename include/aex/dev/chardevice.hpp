#pragma once

#include "aex/dev/charhandle.hpp"
#include "aex/dev/device.hpp"
#include "aex/errno.hpp"
#include "aex/fs.hpp"
#include "aex/mem.hpp"
#include "aex/optional.hpp"
#include "aex/types.hpp"
#include "aex/utility.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Dev {
    class API CharDevice : public Device {
        public:
        CharDevice(const char* name);

        virtual ~CharDevice();

        virtual error_t open(CharHandle* handle, int mode);
        virtual error_t close(CharHandle* handle);

        virtual optional<ssize_t>          read(CharHandle* handle, void* ptr, size_t len);
        virtual optional<ssize_t>          write(CharHandle* handle, const void* ptr, size_t len);
        virtual optional<int>              ioctl(CharHandle* handle, int rq, uint64_t val);
        virtual optional<Mem::MMapRegion*> mmap(Proc::Process* process, void*, size_t len,
                                                int flags, FS::File_SP file, FS::off_t offset);

        virtual bool isatty();
    };

    typedef Mem::SmartPointer<CharDevice> CharDevice_SP;

    API optional<CharHandle_SP> open_char_handle(int id, int mode);
}