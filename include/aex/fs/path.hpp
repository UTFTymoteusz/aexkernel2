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

    /**
     * Checks if a path fits in MAX_LEN.
     * @returns True if the specified path fits, false otherwies.
     */
    bool check_length(const char* path);

    bool ends_with_slash(const char* path);

    bool is_valid(const char* path);
}