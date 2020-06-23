#include "aex/arch/sys/cpu.hpp"
#include "aex/dev/tree/bus.hpp"
#include "aex/dev/tree/tree.hpp"
#include "aex/printk.hpp"

using CPU    = AEX::Sys::CPU;
using Device = AEX::Dev::Tree::Device;

namespace AEX::Dev {
    Tree::Bus* mainbus;

    void detect_pci();
    void detect_ps2();

    void mainbus_init() {
        mainbus = new Tree::Bus("main");

        detect_pci();
        detect_ps2();
    }

    void detect_pci() {
        auto pci = new Device("pci");

        pci->addResource(Device::resource(Device::resource::IO, 0xCF8));
        pci->addResource(Device::resource(Device::resource::IO, 0xCFC));

        mainbus->registerDevice(pci);
    }

    void detect_ps2() {
        CPU::outportb(0x64, 0xAD);
        CPU::outportb(0x64, 0xA7);

        CPU::outportb(0x64, 0xAA);

        for (size_t i = 0; i < 200; i++) {
            if (CPU::inportb(0x60) == 0x55) {
                auto ps2 = new Device("ps2");

                ps2->addResource(Device::resource(Device::resource::IO, 0x60));
                ps2->addResource(Device::resource(Device::resource::IO, 0x64));
                ps2->addResource(Device::resource(Device::resource::IRQ, 1));
                ps2->addResource(Device::resource(Device::resource::IRQ, 12));

                mainbus->registerDevice(ps2);

                break;
            }
        }
    }
}