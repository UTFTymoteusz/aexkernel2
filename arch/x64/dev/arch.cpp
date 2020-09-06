
namespace AEX::Sys::PCI {
    void init();
}

namespace AEX::Dev {
    void arch_drivers_init() {
        Sys::PCI::init();
    }
}