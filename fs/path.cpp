#include "fs/path.hpp"

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

        strncpy(buffer, &((char*) path)[last], min(num, len - last + 1));

        return buffer;
    }
}