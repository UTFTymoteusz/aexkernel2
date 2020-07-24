namespace AEX::Dev {
    namespace PCI {
        void init();
    }

    namespace SATA {
        extern void init();
    }


    void arch_drivers_init() {
        PCI::init();
    }
}