#pragma once

namespace AEX::Sys::ACPI {
    struct rsdp;
    struct xsdp;

    rsdp* find_rsdp();
    xsdp* find_xsdp();

    void init();
}
