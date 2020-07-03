#include "aex/dev/input.hpp"

#include "dev/input.hpp"

namespace AEX::Dev::Input {
    Handle::Handle(int buffer_size) : _buffer(buffer_size) {}

    Handle::~Handle() {
        unregister_handle(this);
    }

    Handle Handle::getHandle(int buffer_size) {
        return Handle(buffer_size);
    }

    void Handle::begin() {
        if (_registered)
            return;

        _registered = true;

        register_handle(this);
    }

    event Handle::readEvent() {
        event _evnt;
        _buffer.read(&_evnt, sizeof(event));
        return _evnt;
    }

    void Handle::writeEvent(event _evnt) {
        if ((size_t) _buffer.writeAvailable() < sizeof(event))
            return;

        _buffer.write(&_evnt, sizeof(event));
    }
}