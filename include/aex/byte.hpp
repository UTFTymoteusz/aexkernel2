#pragma once

#define uint16_bswap(x) ((x & 0xFF00) >> 8) | (x << 8)
#define uint32_bswap(x) \
    ((x & 0xFF000000) >> 24) | ((x & 0x00FF0000) >> 8) | ((x & 0x0000FF00) << 8) | (x << 24)