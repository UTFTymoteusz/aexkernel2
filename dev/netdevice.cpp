#include "aex/dev/netdevice.hpp"

#include "aex/dev.hpp"

namespace AEX::Dev {
    NetDevice::NetDevice(const char* name, Net::link_type_t link_type) : Device(name, DEV_NET) {
        this->link_type = link_type;
    }

    NetDevice::~NetDevice() {}

    error_t NetDevice::send(const void*, size_t, Net::net_type_t) {
        return ENOSYS;
    }

    void NetDevice::setIPv4Address(Net::ipv4_addr addr) {
        info.ipv4.addr      = addr;
        info.ipv4.broadcast = info.ipv4.addr | ~info.ipv4.mask;
    }

    void NetDevice::setIPv4Mask(Net::ipv4_addr addr) {
        info.ipv4.mask      = addr;
        info.ipv4.broadcast = info.ipv4.addr | ~info.ipv4.mask;
    }

    void NetDevice::setIPv4Gateway(Net::ipv4_addr addr) {
        info.ipv4.gateway = addr;
    }

    void NetDevice::setMetric(int metric) {
        this->metric = metric;
    }

    Mem::SmartPointer<NetDevice> get_net_device(int id) {
        auto device = devices.get(id);
        if (!device || device->type != DEV_NET)
            return devices.get(-1);

        return device;
    }
}