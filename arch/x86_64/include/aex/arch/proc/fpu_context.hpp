#pragma once

#include "aex/types.hpp"
#include "aex/utility.hpp"

namespace AEX::Proc {
    struct API fpu_context {
        uint16_t fcw;
        uint16_t fsw;
        uint8_t  ftw;
        uint8_t  reserved0;
        uint16_t fop;
        uint32_t fip;
        uint16_t fcs;
        uint16_t reserved1;

        uint32_t fdp;
        uint16_t fds;
        uint16_t reserved2;
        uint32_t mxcsr;
        uint32_t mxcsr_mask;

        union {
            uint8_t st0[16];
            uint8_t mm0[16];
        };
        union {
            uint8_t st1[16];
            uint8_t mm1[16];
        };
        union {
            uint8_t st2[16];
            uint8_t mm2[16];
        };
        union {
            uint8_t st3[16];
            uint8_t mm3[16];
        };
        union {
            uint8_t st4[16];
            uint8_t mm4[16];
        };
        union {
            uint8_t st5[16];
            uint8_t mm5[16];
        };
        union {
            uint8_t st6[16];
            uint8_t mm6[16];
        };
        union {
            uint8_t st7[16];
            uint8_t mm7[16];
        };

        uint8_t xmm0[16];
        uint8_t xmm1[16];
        uint8_t xmm2[16];
        uint8_t xmm3[16];
        uint8_t xmm4[16];
        uint8_t xmm5[16];
        uint8_t xmm6[16];
        uint8_t xmm7[16];
        uint8_t xmm8[16];
        uint8_t xmm9[16];
        uint8_t xmm10[16];
        uint8_t xmm11[16];
        uint8_t xmm12[16];
        uint8_t xmm13[16];
        uint8_t xmm14[16];
        uint8_t xmm15[16];

        uint8_t reserved3[48];
        uint8_t available[48];
    } PACKED;

    static_assert(sizeof(fpu_context) == 512);
}