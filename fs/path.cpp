#include "aex/fs/path.hpp"

#include "aex/math.hpp"
#include "aex/string.hpp"

// clang-format off
#include "aex/printk.hpp"
// clang-format on

namespace AEX::FS::Path {
    Walker::Walker(const char* path) {
        _path = path;
    }

    char* Walker::next() {
        if (!_path[_index] || _too_long)
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

        if (chars_this_piece == 0)
            return nullptr;

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

    char* canonize_path(const char* path, const char* base_path, char* buffer, size_t buffer_len) {
        size_t index = 0;

        if (buffer_len < 2)
            return nullptr;

        buffer_len--;

        buffer[0]          = '/';
        buffer[1]          = '\0';
        buffer[buffer_len] = '\0';

        int base_len = strlen(base_path);

        if (base_path && base_len > 0 && path[0] != '/') {
            if (base_len > buffer_len)
                return nullptr;

            memcpy(buffer, base_path, base_len);

            index += base_len;

            buffer[index] = '\0';

            if (buffer[index - 1] == '/')
                index--;
        }

        for (auto walker = FS::Path::Walker(path); auto piece = walker.next();) {
            if (strcmp(piece, ".") == 0)
                continue;
            else if (strcmp(piece, "..") == 0) {
                while (index > 0 && buffer[index] != '/')
                    index--;

                if (index == 0)
                    continue;

                buffer[index] = '\0';

                continue;
            }

            int piece_len = strlen(piece);

            if (index + piece_len >= buffer_len)
                return nullptr;

            buffer[index] = '/';
            index++;
            strncpy(&buffer[index], piece, MAX_PATH_LEN);

            index += piece_len;
            buffer[index] = '\0';
        }

        if (ends_with_slash(path)) {
            if (index >= buffer_len)
                return nullptr;

            buffer[index]     = '/';
            buffer[index + 1] = '\0';
        }

        return buffer;
    }
}