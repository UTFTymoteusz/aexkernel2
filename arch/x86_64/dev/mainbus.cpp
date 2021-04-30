#include "aex/arch/sys/cpu.hpp"
#include "aex/dev/tree.hpp"
#include "aex/printk.hpp"

using CPU    = AEX::Sys::CPU;
using Bus    = AEX::Dev::Tree::Bus;
using Device = AEX::Dev::Tree::Device;

namespace AEX::Dev {
    Tree::Bus* mainbus;

    void detect_pci();
    void detect_ps2();
    void detect_rtc();

    void mainbus_init() {
        using namespace AEX::Dev::Tree;

        mainbus = new Bus("main");

        detect_pci();
        detect_ps2();
        detect_rtc();
    }

    void detect_pci() {
        using namespace AEX::Dev::Tree;

        auto pci = new Device("pci", nullptr);

        pci->add(resource(resource::RES_IO, 0xCF8));
        pci->add(resource(resource::RES_IO, 0xCFC));

        mainbus->registerDevice(pci);
    }

    void detect_ps2() {
        using namespace AEX::Dev::Tree;

        CPU::outb(0x64, 0xAD);
        CPU::outb(0x64, 0xA7);
        CPU::outb(0x64, 0xAA);

        for (size_t i = 0; i < 200; i++) {
            if (CPU::inb(0x60) != 0x55)
                continue;

            auto ps2 = new Device("ps2", nullptr);

            ps2->add(resource(resource::RES_IO, 0x60));
            ps2->add(resource(resource::RES_IO, 0x64));
            ps2->add(resource(resource::RES_IRQ, 1));
            ps2->add(resource(resource::RES_IRQ, 12));

            mainbus->registerDevice(ps2);

            break;
        }
    }

    void detect_rtc() {
        using namespace AEX::Dev::Tree;

        auto rtc = new Device("rtc", nullptr);

        rtc->add(resource(resource::RES_IO, 0x70, 2));
        rtc->add(resource(resource::RES_IRQ, 8));

        mainbus->registerDevice(rtc);
    }
}