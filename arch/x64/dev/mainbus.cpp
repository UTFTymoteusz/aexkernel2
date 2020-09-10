#include "aex/arch/sys/cpu.hpp"
#include "aex/dev/tree.hpp"
#include "aex/printk.hpp"

using CPU    = AEX::Sys::CPU;
using Device = AEX::Dev::Tree::Device;

namespace AEX::Dev {
    Tree::Bus* mainbus;

    void detect_pci();
    void detect_ps2();
    void detect_rtc();

    void mainbus_init() {
        mainbus = new Tree::Bus("main");

        detect_pci();
        detect_ps2();
        detect_rtc();
    }

    void detect_pci() {
        using namespace AEX::Dev::Tree;

        auto pci = new Device("pci", nullptr);

        pci->addResource(resource(resource::IO, 0xCF8));
        pci->addResource(resource(resource::IO, 0xCFC));

        mainbus->registerDevice(pci);
    }

    void detect_ps2() {
        using namespace AEX::Dev::Tree;

        CPU::outportb(0x64, 0xAD);
        CPU::outportb(0x64, 0xA7);
        CPU::outportb(0x64, 0xAA);

        for (size_t i = 0; i < 200; i++) {
            if (CPU::inportb(0x60) != 0x55)
                continue;

            auto ps2 = new Device("ps2", nullptr);

            ps2->addResource(resource(resource::IO, 0x60));
            ps2->addResource(resource(resource::IO, 0x64));
            ps2->addResource(resource(resource::IRQ, 1));
            ps2->addResource(resource(resource::IRQ, 12));

            mainbus->registerDevice(ps2);

            break;
        }
    }

    void detect_rtc() {
        using namespace AEX::Dev::Tree;

        auto rtc = new Device("rtc", nullptr);

        rtc->addResource(resource(resource::IO, 0x70, 2));
        rtc->addResource(resource(resource::IRQ, 8));

        mainbus->registerDevice(rtc);
    }
}