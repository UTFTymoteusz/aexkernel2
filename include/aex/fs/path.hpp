#pragma once

#include <stddef.h>

namespace AEX::FS::Path {
    constexpr auto MAX_PATH_LEN     = 2048;
    constexpr auto MAX_FILENAME_LEN = 256;
    constexpr auto MAX_DEPTH        = 256;

    class Walker {
      public:
        Walker(const char* path);

        const char* next();

        int  level();
        bool isPieceTooLong();
        bool isFinal();

      private:
        char _buffer[MAX_FILENAME_LEN];

        int         _index = 0;
        const char* _path;
        bool        _too_long = false;

        int _level  = 0;
        int _levels = 0;
    };

    /**
     * Gets the filename or a directory name in a path.
     * @param buffer Destination buffer.
     * @param path   Path.
     * @param num    Size of destination buffer.
     * @returns Pointer to destination buffer.
     */
    char* get_filename(char* buffer, const char* path, size_t num);

    /**
     * Checks if a path fits in MAX_PATH_LEN.
     * @returns True if the specified path fits, false otherwies.
     */
    bool check_length(const char* path);

    bool ends_with_slash(const char* path);

    bool is_valid(const char* path);

    char* canonize_path(const char* path, const char* base_path, char* buffer, size_t buffer_len);

    int count_levels(const char* path);
}