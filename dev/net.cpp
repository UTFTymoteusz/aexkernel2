#include "aex/dev/net.hpp"

#include "aex/dev/dev.hpp"
#include "aex/net/linklayer.hpp"


namespace AEX::Dev {
    Net::Net(const char* name, net_type_t net_type) : Device(name, type_t::NET) {
        this->net_type = net_type;
    }

    Net::~Net() {}

    error_t Net::send(const void*, size_t) {
        return error_t::ENOSYS;
    }

    void Net::receive(const void* buffer, size_t len) {
        switch (net_type) {
        case net_type_t::ETHERNET:
            AEX::Net::parse(id, AEX::Net::llayer_type_t::ETHERNET, buffer, len);
            break;
        }
    }

    Mem::SmartPointer<Net> get_net_device(int id) {
        auto device = devices.get(id);
        if (!device.isValid() || device->type != Device::type_t::NET)
            return devices.get(-1);

        return device;
    }
}