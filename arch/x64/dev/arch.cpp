namespace AEX::Dev {
    namespace PCI {
        void init();
    }

    void arch_drivers_init() {
        PCI::init();
    }
}