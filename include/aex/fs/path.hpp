#pragma once

#include <stddef.h>

namespace AEX::FS::Path {
    constexpr auto MAX_LEN = 1024;

    /**
     * Gets the filename or a directory name in a path.
     * @param buffer Destination buffer.
     * @param path   Path.
     * @param num    Size of destination buffer.
     * @returns Pointer to destination buffer.
     */
    char* get_filename(char* buffer, const char* path, size_t num);
}