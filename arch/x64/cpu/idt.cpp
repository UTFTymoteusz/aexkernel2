#include "cpu/idt.hpp"

#include "aex/string.hpp"

namespace AEX::Sys {
    extern "C" char exc_array;
    extern "C" char irq_array;

    extern "C" char irq_spurious;

    IDTEntry init_IDT[256];

    void IDTEntry::setOffset(size_t offset) {
        offset_0 = offset & 0xFFFF;
        offset_1 = (offset >> 16) & 0xFFFF;
        offset_2 = (offset >> 32) & 0xFFFFFFFF;
    }

    void IDTEntry::setType(uint8_t type) {
        attributes &= 0xF0;
        attributes |= type;
    }

    void IDTEntry::setSelector(uint8_t selector) {
        this->selector = selector;
    }

    void IDTEntry::setPresent(bool present) {
        attributes = present ? (attributes | 0x80) : (attributes & 0x7F);
    }

    void setup_idt() {
        memset(init_IDT, 0, sizeof(init_IDT));

        size_t* _exc_array = (size_t*) &exc_array;

        for (int i = 0; i < 32; i++) {
            init_IDT[i].setOffset(_exc_array[i]);
            init_IDT[i].setSelector(0x08);
            init_IDT[i].setType(0x0E);
            init_IDT[i].setPresent(true);
        }

        size_t* _irq_array = (size_t*) &irq_array;

        for (int i = 0; i < 32; i++) {
            init_IDT[i + 32].setOffset(_irq_array[i]);
            init_IDT[i + 32].setSelector(0x08);
            init_IDT[i + 32].setType(0x0E);
            init_IDT[i + 32].setPresent(true);
        }

        size_t spurious = (size_t) &irq_spurious;

        init_IDT[255].setOffset(spurious);
        init_IDT[255].setSelector(0x08);
        init_IDT[255].setType(0x0E);
        init_IDT[255].setPresent(true);
    }

    void load_idt(IDTEntry* idt, size_t entry_count) {
        uint8_t idt_descriptor[10];

        uint16_t* size = (uint16_t*) &idt_descriptor[0];
        uint64_t* addr = (uint64_t*) &idt_descriptor[2];

        *size = entry_count * sizeof(IDTEntry) - 1;
        *addr = (uint64_t) idt;

        asm volatile("lidt [%0]" : : "r"(&idt_descriptor) : "memory");
    }
}