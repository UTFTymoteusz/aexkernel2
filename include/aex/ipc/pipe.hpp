#pragma once

#include "aex/errno.hpp"
#include "aex/fs/file.hpp"
#include "aex/mem/circularbuffer.hpp"
#include "aex/mem/smartptr.hpp"

#include <stdint.h>

namespace AEX::IPC {
    static constexpr auto PIPE_SIZE = 4096;

    class Pipe {
        public:
        static error_t create(FS::File_SP& w, FS::File_SP& r);

        Mem::CircularBuffer buffer = Mem::CircularBuffer(PIPE_SIZE);
    };

    class PipeWriter : public FS::File {
        public:
        PipeWriter(Mem::SmartPointer<Pipe> pipe);

        optional<Mem::SmartPointer<FS::File>> open(const char* path) = delete;

        optional<ssize_t>     write(void* buf, size_t count);
        optional<FS::File_SP> dup();

        private:
        Mem::SmartPointer<Pipe> _pipe;
    };

    class PipeReader : public FS::File {
        public:
        PipeReader(Mem::SmartPointer<Pipe> pipe);

        optional<Mem::SmartPointer<FS::File>> open(const char* path) = delete;

        optional<ssize_t>     read(void* buf, size_t count);
        optional<FS::File_SP> dup();

        private:
        Mem::SmartPointer<Pipe> _pipe;
    };
}
