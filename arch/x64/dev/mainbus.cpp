#include "aex/dev/bus.hpp"
#include "aex/dev/tree.hpp"

namespace AEX::Dev {
    Bus* mainbus;

    void mainbus_init() {
        mainbus = new Bus("main");

        auto pci = new Device("pci");

        pci->addResource(Device::resource(Device::resource::IO, 0xCF8));
        pci->addResource(Device::resource(Device::resource::IO, 0xCFC));

        mainbus->registerDevice(pci);
    }
}