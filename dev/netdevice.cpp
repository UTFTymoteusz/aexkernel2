#include "aex/dev/netdevice.hpp"

#include "aex/dev/dev.hpp"
#include "aex/net/linklayer.hpp"

namespace AEX::Dev {
    NetDevice::NetDevice(const char* name, Net::link_type_t link_type) : Device(name, type_t::NET) {
        this->link_type = link_type;
    }

    NetDevice::~NetDevice() {}

    error_t NetDevice::send(const void*, size_t) {
        return error_t::ENOSYS;
    }

    void NetDevice::receive(const void* buffer, size_t len) {
        Net::parse(id, link_type, buffer, len);
    }

    void NetDevice::setIPv4Address(Net::ipv4_addr addr) {
        ipv4_addr      = addr;
        ipv4_broadcast = ipv4_addr | ~ipv4_mask;
    }

    void NetDevice::setIPv4Mask(Net::ipv4_addr addr) {
        ipv4_mask      = addr;
        ipv4_broadcast = ipv4_addr | ~ipv4_mask;
    }

    Mem::SmartPointer<NetDevice> get_net_device(int id) {
        auto device = devices.get(id);
        if (!device.isValid() || device->type != Device::type_t::NET)
            return devices.get(-1);

        return device;
    }
}