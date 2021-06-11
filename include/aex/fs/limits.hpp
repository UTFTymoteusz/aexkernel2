#pragma once

namespace AEX::FS {
    // The maximum length of a file path, including the terminating null byte.
    constexpr auto PATH_MAX = 2048;
    // The maximum length of a file path component, not including the terminating null byte.
    constexpr auto NAME_MAX = 255;
    // The maximum amount of components in a file path.
    constexpr auto DEPTH_MAX = 256;
}