#include "aex/ipc/pipe.hpp"

namespace AEX::IPC {
    error_t Pipe::create(FS::File_SP& r, FS::File_SP& w) {
        auto pipe = new Pipe();
        auto sptr = Mem::SmartPointer(pipe);

        r = FS::File_SP(new PipeReader(sptr));
        w = FS::File_SP(new PipeWriter(sptr));

        return ENONE;
    }

    PipeReader::PipeReader(Mem::SmartPointer<Pipe> pipe) : _pipe(pipe) {}

    optional<uint32_t> PipeReader::read(void* buf, uint32_t count) {
        return _pipe->buffer.read(buf, count);
    }

    optional<FS::File_SP> PipeReader::dup() {
        return FS::File_SP(new PipeReader(_pipe));
    }

    PipeWriter::PipeWriter(Mem::SmartPointer<Pipe> pipe) : _pipe(pipe) {}

    optional<uint32_t> PipeWriter::write(void* buf, uint32_t count) {
        return _pipe->buffer.write(buf, count);
    }

    optional<FS::File_SP> PipeWriter::dup() {
        return FS::File_SP(new PipeWriter(_pipe));
    }
}