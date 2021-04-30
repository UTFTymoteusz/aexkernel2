#include "aex/sys/pci/resolv.hpp"

namespace AEX::Sys::PCI {
    const char* resolve(uint16_t _class, uint16_t _subclass, uint16_t _prog_if) {
        switch (_class) {
        case 0x00:
            switch (_subclass) {
            case 0x00:
                return "Non-VGA-Compatible Unclassified";
            case 0x01:
                return "VGA-Compatible Unclassified";
            case 0x05:
                return "Image Coprocessor";
            }
            return "Unclassified Device";
        case 0x01:
            switch (_subclass) {
            case 0x00:
                return "SCSI Bus Controller";
            case 0x01:
                return "IDE Controller";
            case 0x02:
                return "Floppy Disk Controller";
            case 0x03:
                return "IPI Bus Controller";
            case 0x04:
                return "RAID Controller";
            case 0x05:
                return "ATA Controller";
            case 0x06:
                return "SATA Controller";
            case 0x07:
                return "SAS Controller";
            case 0x08:
                switch (_prog_if) {
                case 0x01:
                    return "NVMHCI Controller";
                case 0x02:
                    return "NVMe Controller";
                }
            }
            return "Mass Storage Controller";
        case 0x02:
            switch (_subclass) {
            case 0x00:
                return "Ethernet Controller";
            case 0x01:
                return "Token Ring Controller";
            case 0x02:
                return "FDDI Controller";
            case 0x03:
                return "ATM Controller";
            case 0x04:
                return "ISDN Controller";
            case 0x05:
                return "WorldFip Controller";
            case 0x06:
                return "PICMG 2.14 Multi Computing Controller";
            case 0x07:
                return "InfiniBand Controller";
            case 0x08:
                return "Fabric Controller";
            }
            return "Network Controller";
        case 0x03:
            switch (_subclass) {
            case 0x00:
                switch (_prog_if) {
                case 0x00:
                    return "VGA Controller";
                case 0x01:
                    return "8514 Controller";
                }
                return "VGA Compatible Controller";
            case 0x01:
                return "XGA Controller";
            case 0x02:
                return "3D Controller";
            }
            return "Display Controller";
        case 0x04:
            switch (_subclass) {
            case 0x00:
                return "Multimedia Video Controller";
            case 0x01:
                return "Multimedia Audio Controller";
            case 0x02:
                return "Computer Telephony Device";
            case 0x03:
                return "Audio Device";
            }
            return "Multimedia Controller";
        case 0x05:
            switch (_subclass) {
            case 0x00:
                return "RAM Controller";
            case 0x01:
                return "Flash Controller";
            }
            return "Memory Controller";
        case 0x06:
            switch (_subclass) {
            case 0x00:
                return "Host Bridge";
            case 0x01:
                return "ISA Bridge";
            case 0x02:
                return "EISA Bridge";
            case 0x03:
                return "MCA Bridge";
            case 0x04:
                return "PCI2PCI Bridge";
            case 0x05:
                return "PCMCIA Bridge";
            case 0x06:
                return "NuBus Bridge";
            case 0x07:
                return "CardBus Bridge";
            case 0x08:
                return "RACEway Bridge";
            case 0x09:
                return "PCI2PCI Bridge";
            case 0x0A:
                return "InfiniBand2PCI Bridge";
            }
            return "Bridge";
        case 0x07:
            switch (_subclass) {
            case 0x00:
                switch (_prog_if) {
                case 0x00:
                    return "8250 Serial Controller";
                case 0x01:
                    return "16450 Serial Controller";
                case 0x02:
                    return "16550 Serial Controller";
                case 0x03:
                    return "16650 Serial Controller";
                case 0x04:
                    return "16750 Serial Controller";
                case 0x05:
                    return "16850 Serial Controller";
                case 0x06:
                    return "16950 Serial Controller";
                }
                return "Serial Controller";
            case 0x01:
                switch (_prog_if) {
                case 0x00:
                    return "Standard Parallel Controller";
                case 0x01:
                    return "Bi-Directional Parallel Controller";
                case 0x02:
                    return "ECP Parallel Controller";
                case 0x03:
                    return "16650 Serial Controller";
                case 0x04:
                    return "IEEE 1284 Controller";
                case 0xFE:
                    return "IEEE 1284 Target Device";
                }
                return "Parallel Controller";
            case 0x02:
                return "Multiport Serial Controller";
            case 0x03:
                switch (_prog_if) {
                case 0x00:
                    return "Generic Modem";
                case 0x01:
                    return "16450 Modem";
                case 0x02:
                    return "16550 Modem";
                case 0x03:
                    return "16650 Modem";
                case 0x04:
                    return "16750 Modem";
                }
                return "Modem";
            case 0x04:
                return "GPIB Controller";
            case 0x05:
                return "Smart Card Controller";
            }
            return "Communication Controller";
        case 0x08:
            switch (_subclass) {
            case 0x00:
                return "PIC";
            case 0x01:
                return "DMA Controller";
            case 0x02:
                return "Timer";
            case 0x03:
                return "RTC";
            case 0x04:
                return "PCI Hot-Plug Controller";
            case 0x05:
                return "SD Host Controller";
            case 0x06:
                return "IOMMU Controller";
            case 0x99:
                return "Timing Card";
            }
            return "System Peripheral";
        case 0x09:
            switch (_subclass) {
            case 0x00:
                return "Keyboard Controller";
            case 0x01:
                return "Digitizer Pen";
            case 0x02:
                return "Mouse Controller";
            case 0x03:
                return "Scanner Controller";
            case 0x04:
                switch (_prog_if) {
                case 0x00:
                    return "Generic Gameport Controller";
                case 0x10:
                    return "Extended Gameport Controller";
                }
                return "Gameport Controller";
            }
            return "Input Device Controller";
        case 0x0A:
            switch (_subclass) {
            case 0x00:
                return "Generic Docking Station";
            }
            return "Docking Station";
        case 0x0B:
            switch (_subclass) {
            case 0x00:
                return "386 Processor";
            case 0x01:
                return "486 Processor";
            case 0x02:
                return "Pentium Processor";
            case 0x03:
                return "Pentium Pro Processor";
            case 0x10:
                return "Alpha Processor";
            case 0x20:
                return "PowerPC Processor";
            case 0x30:
                return "MIPS Processor";
            case 0x40:
                return "Co-Processor";
            }
            return "Processor";
        case 0x0C:
            switch (_subclass) {
            case 0x00:
                switch (_prog_if) {
                case 0x00:
                    return "FireWire Controller";
                case 0x10:
                    return "FireWire OHCI Controller";
                }
                return "FireWire Controller";
            case 0x01:
                return "ACCESS Bus";
            case 0x02:
                return "SSA";
            case 0x03:
                switch (_prog_if) {
                case 0x00:
                    return "USB UHCI Controller";
                case 0x10:
                    return "USB OHCI Controller";
                case 0x20:
                    return "USB EHCI Controller";
                case 0x30:
                    return "USB XHCI Controller";
                case 0x40:
                    return "USB4 Host Interface";
                case 0xFE:
                    return "USB Device";
                }
                return "USB Controller";
            case 0x04:
                return "Fibre Channel";
            case 0x05:
                return "SMBus";
            case 0x06:
                return "Infiniband";
            case 0x07:
                switch (_prog_if) {
                case 0x00:
                    return "IPMI SMIC";
                case 0x01:
                    return "IPMI KCS";
                case 0x02:
                    return "IPMI BT";
                }
                return "IPMI Interface";
            case 0x08:
                return "SERCOS Interface";
            case 0x09:
                return "CANBUS";
            }
            return "Bridge";
        case 0x0D:
            switch (_subclass) {
            case 0x00:
                return "IRDA Controller";
            case 0x01:
                return "Consumer IR Controller";
            case 0x10:
                return "RF Controller";
            case 0x11:
                return "Bluetooth Controller";
            case 0x12:
                return "Broadband Controller";
            case 0x20:
                return "802.1a Controller";
            case 0x21:
                return "802.1b Controller";
            }
            return "Wireless Controller";
        case 0x0E:
            return "I2O";
        case 0x0F:
            switch (_subclass) {
            case 0x01:
                return "Satellite TV Controller";
            case 0x02:
                return "Satellite Audio Controller";
            case 0x03:
                return "Satellite Voice Controller";
            case 0x04:
                return "Satellite Data Controller";
            }
            return "Satellite Communication Controller";
        case 0x10:
            switch (_subclass) {
            case 0x00:
                return "Network and Computing Encryption Device";
            case 0x10:
                return "Entertainment Encryption Device";
            }
            return "Encryption Controller";
        case 0x11:
            switch (_subclass) {
            case 0x00:
                return "DPIO Module";
            case 0x01:
                return "Performance Counters";
            case 0x10:
                return "Communication Synchronizer";
            case 0x20:
                return "Signal Processing Management";
            }
            return "Signal Processing Controller";
        case 0x12:
            switch (_subclass) {
            case 0x00:
                return "Processing Accelerator";
            case 0x01:
                return "AI Inference Accelerator";
            }
            return "Processing Accelerator";
        case 0x13:
            return "Non-Essential Instrumentation";
        case 0x40:
            return "Co-Processor";
        }
        return "Unknown";
    }
}