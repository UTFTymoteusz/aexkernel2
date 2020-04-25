/*
 * mcore.hpp: Handles initializing multiple cores, mostly
 */
#pragma once

#include "lib/rcparray.hpp"
#include "sys/cpu.hpp"

namespace AEX::Sys::MCore {
    extern RCPArray<CPU> CPUs;

    void init();
};