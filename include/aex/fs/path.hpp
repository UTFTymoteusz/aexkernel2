#pragma once

#include "aex/utility.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::FS {
    constexpr auto PATH_MAX  = 2048;
    constexpr auto NAME_MAX  = 256;
    constexpr auto DEPTH_MAX = 256;

    class API Walker {
        public:
        Walker(const char* path);

        const char* next();
        int         level();
        bool        overflow();
        bool        final();

        private:
        char m_buffer[NAME_MAX];

        int         m_index = 0;
        const char* m_path;
        bool        m_overflow = false;
        int         m_level    = 0;
        int         m_levels   = 0;
    };

    /**
     * Gets the filename or a directory name in a path.
     * @param buffer Destination buffer.
     * @param path   Path.
     * @param num    Size of destination buffer.
     * @param noext  Whether to strip the extension.
     * @returns Pointer to the destination buffer.
     **/
    API char* get_filename(char* buffer, const char* path, size_t num, bool noext = false);

    /**
     * Gets the extension in a path. Omits the dot.
     * @param buffer Destination buffer.
     * @param path   Path.
     * @param num    Size of destination buffer.
     * @returns Pointer to the destination buffer.
     **/
    API char* get_extension(char* buffer, const char* path, size_t num);

    /**
     * Checks if a path fits within MAX_PATH_LEN.
     * @returns True if the specified path fits, false otherwise.
     **/
    API bool check_length(const char* path);

    API bool  ends_with_slash(const char* path);
    API bool  is_valid(const char* path);
    API char* canonize_path(const char* path, const char* base_path, char* buffer,
                            size_t buffer_len);
    API int   count_levels(const char* path);
}