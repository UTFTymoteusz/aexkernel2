#include "sys/cpu/idt.hpp"

#include "aex/string.hpp"

namespace AEX::Sys {
    extern "C" char exc_array;
    extern "C" char irq_array;

    extern "C" char irq_spurious;

    IDTEntry init_IDT[256];

    IDTEntry& IDTEntry::setOffset(size_t offset) {
        offset_0 = offset & 0xFFFF;
        offset_1 = (offset >> 16) & 0xFFFF;
        offset_2 = (offset >> 32) & 0xFFFFFFFF;

        return *this;
    }

    IDTEntry& IDTEntry::setOffset(void* offset) {
        return setOffset((size_t) offset);
    }

    IDTEntry& IDTEntry::setType(uint8_t type) {
        attributes &= 0xF0;
        attributes |= type;

        return *this;
    }

    IDTEntry& IDTEntry::setSelector(uint8_t selector) {
        this->selector = selector;
        return *this;
    }

    IDTEntry& IDTEntry::setPresent(bool present) {
        attributes = present ? (attributes | 0x80) : (attributes & 0x7F);
        return *this;
    }

    IDTEntry& IDTEntry::setIST(uint8_t ist) {
        this->ist = ist;
        return *this;
    }

    void setup_idt() {
        static bool ready = false;
        if (ready)
            return;

        ready = true;

        memset(init_IDT, 0, sizeof(init_IDT));

        size_t* m_exc_array = (size_t*) &exc_array;
        for (int i = 0; i < 32; i++)
            init_IDT[i]
                .setOffset(m_exc_array[i])
                .setSelector(0x08)
                .setType(0x0E)
                .setIST(0)
                .setPresent(true);

        size_t* m_irq_array = (size_t*) &irq_array;
        for (int i = 0; i < 32; i++)
            init_IDT[i + 32]
                .setOffset(m_irq_array[i])
                .setSelector(0x08)
                .setType(0x0E)
                .setIST(0)
                .setPresent(true);

        // clang-format off
        init_IDT[255]
            .setOffset((size_t) &irq_spurious)
            .setSelector(0x08)
            .setType(0x0E)
            .setIST(0)
            .setPresent(true);
        // clang-format on
    }

    void set_idt_ists() {
        for (int i = 0; i < 32; i++)
            init_IDT[i].setIST(1);

        for (int i = 32; i < 64; i++)
            init_IDT[i].setIST(2);

        init_IDT[255].setIST(2);
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