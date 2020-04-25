#include "cpu/idt.hpp"

namespace AEX::Sys {
    void IDTEntry::setOffset(size_t offset) {
        offset_0 = offset & 0xFFFF;
        offset_1 = (offset >> 16) & 0xFFFF;
        offset_2 = (offset >> 32) & 0xFFFFFFFF;
    }

    void IDTEntry::setType(uint8_t type) {
        attributes &= 0xF0;
        attributes |= type;
    }

    void IDTEntry::setSelector(uint8_t selector) { this->selector = selector; }

    void IDTEntry::setPresent(bool present) {
        attributes = present ? (attributes | 0x80) : (attributes & 0x7F);
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