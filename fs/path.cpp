#include "aex/fs/path.hpp"

#include "aex/math.hpp"
#include "aex/string.hpp"

namespace AEX::FS::Path {
    Walker::Walker(const char* path) {
        _path = path;
    }

    char* Walker::next() {
        if (!_path[_index])
            return nullptr;

        while (_path[_index] == '/')
            _index++;

        int chars_this_piece = 0;

        while (_path[_index] && _path[_index] != '/') {
            if (chars_this_piece >= MAX_FILENAME_LEN - 1) {
                _too_long = true;
                return nullptr;
            }

            _buffer[chars_this_piece] = _path[_index];

            chars_this_piece++;
            _index++;
        }

        _buffer[chars_this_piece] = '\0';

        return _buffer;
    }

    int Walker::level() {
        return _level;
    }

    bool Walker::is_piece_too_long() {
        return _too_long;
    }

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
        return strlen(path) + 1 < MAX_PATH_LEN;
    }

    bool ends_with_slash(const char* path) {
        return path[strlen(path) - 1] == '/';
    }

    bool is_valid(const char* path) {
        if (!check_length(path))
            return false;

        if (path[0] != '/')
            return false;

        int index = 1;

        while (path[index] == '/')
            index++;

        int chars_this_piece = 0;

        while (path[index]) {
            while (path[index] && path[index] != '/') {
                if (chars_this_piece >= MAX_FILENAME_LEN - 1)
                    return false;

                chars_this_piece++;
                index++;
            }

            index++;
        }

        return true;
    }
}