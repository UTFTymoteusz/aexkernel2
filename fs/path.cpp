#include "aex/fs/path.hpp"

#include "aex/math.hpp"
#include "aex/string.hpp"

namespace AEX::FS::Path {
    char* get_filename(char* buffer, const char* path, size_t num) {
        int len = strlen(path);
        if (len <= 1)
            return (char*) path;

        if (path[len - 1] == '/')
            len--;

        int last = 0;

        for (int i = 0; i < len; i++) {
            if (path[i] == '/')
                last = i + 1;
        }

        strncpy(buffer, &((char*) path)[last], min((int) num, len - last + 1));

        return buffer;
    }

    bool check_length(const char* path) {
        return strlen(path) + 1 < MAX_LEN;
    }

    bool ends_with_slash(const char* path) {
        return path[strlen(path) - 1] == '/';
    }

    bool is_valid(const char* path) {
        if (!check_length(path))
            return false;

        if (path[0] != '/')
            return false;

        return true;
    }
}