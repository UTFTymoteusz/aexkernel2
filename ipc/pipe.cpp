#include "aex/ipc/pipe.hpp"

namespace AEX::IPC {
    error_t Pipe::create(FS::File_SP& r, FS::File_SP& w) {
        auto pipe = new Pipe();
        auto sptr = Mem::SmartPointer(pipe);

        r = FS::File_SP(new PipeReader(sptr));
        w = FS::File_SP(new PipeWriter(sptr));

        return ENONE;
    }

    PipeWriter::PipeWriter(Mem::SmartPointer<Pipe> pipe) : _pipe(pipe) {}

    optional<ssize_t> PipeWriter::write(const void* buf, size_t count) {
        if (count <= PIPE_BUF)
            return _pipe->buffer.write((const char*) buf, count, true);

        return _pipe->buffer.write((const char*) buf, count);
    }

    PipeReader::PipeReader(Mem::SmartPointer<Pipe> pipe) : _pipe(pipe) {}

    optional<ssize_t> PipeReader::read(void* buf, size_t count) {
        return _pipe->buffer.read((char*) buf, count, 1);
    }
}