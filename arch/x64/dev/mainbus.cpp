#include "aex/dev/tree/bus.hpp"
#include "aex/dev/tree/tree.hpp"

namespace AEX::Dev {
    using Device = Tree::Device;

    Tree::Bus* mainbus;

    void mainbus_init() {
        mainbus = new Tree::Bus("main");

        auto pci = new Device("pci");

        pci->addResource(Device::resource(Device::resource::IO, 0xCF8));
        pci->addResource(Device::resource(Device::resource::IO, 0xCFC));

        mainbus->registerDevice(pci);
    }
}