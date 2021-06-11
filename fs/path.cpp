#include "aex/fs/path.hpp"

#include "aex/math.hpp"
#include "aex/string.hpp"

// clang-format off
#include "aex/printk.hpp"
// clang-format on

namespace AEX::FS {
    Walker::Walker(const char* path) {
        m_path   = path;
        m_levels = count_levels(path);
    }

    const char* Walker::next() {
        char* piece = strntokp_r(m_buffer, NAME_MAX, m_level == 0 ? m_path : nullptr, "/", &m_path);
        if (piece)
            m_level++;

        return piece;
    }

    int Walker::level() {
        return m_level;
    }

    bool Walker::final() {
        return m_level == m_levels;
    }

    char* get_filename(char* buffer, const char* path, size_t num, bool noext) {
        size_t len = strlen(path);
        if (len <= 1)
            return (char*) path;

        if (path[len - 1] == '/')
            len--;

        size_t last = 0;

        for (size_t i = 0; i < len; i++) {
            if (path[i] == '/')
                last = i + 1;
        }

        if (noext) {
            size_t newlen = 0;
            for (size_t i = 0; i < len; i++) {
                if (path[i] == '.' || path[i] == '\0')
                    break;

                newlen++;
            }

            len = newlen;
        }

        strlcpy(buffer, &((char*) path)[last], min(num, len - last + 1));
        return buffer;
    }

    char* get_extension(char* buffer, const char* path, size_t num) {
        size_t len = strlen(path);
        if (len <= 1) {
            buffer[0] = '\0';
            return buffer;
        }

        char* dot = strrchr(buffer, '.');
        if (!dot) {
            buffer[0] = '\0';
            return buffer;
        }

        strlcpy(buffer, dot + 1, min(num, strlen(dot + 1)));
        return buffer;
    }

    bool check_length(const char* path) {
        return strlen(path) + 1 <= PATH_MAX;
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
                if (chars_this_piece >= NAME_MAX)
                    return false;

                chars_this_piece++;
                index++;
            }

            while (path[index] == '/')
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

        size_t base_len = strlen(base_path);

        if (base_path && base_len > 0 && path[0] != '/') {
            if (base_len > buffer_len)
                return nullptr;

            memcpy(buffer, base_path, base_len);

            index += base_len;

            buffer[index] = '\0';

            if (buffer[index - 1] == '/')
                index--;
        }

        for (auto walker = FS::Walker(path); auto piece = walker.next();) {
            if (strcmp(piece, ".") == 0) {
                continue;
            }
            else if (strcmp(piece, "..") == 0) {
                while (index > 0 && buffer[index] != '/')
                    index--;

                if (index == 0)
                    continue;

                buffer[index] = '\0';
                continue;
            }

            size_t piece_len = strlen(piece);

            if (index + piece_len >= buffer_len)
                return nullptr;

            buffer[index] = '/';
            index++;
            strlcpy(&buffer[index], piece, PATH_MAX);

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

    int count_levels(const char* path) {
        int         total = 0;
        const char* ptr;

        while (strntokp_r((char*) 0x01, 0, path, "/", &ptr)) {
            path = nullptr;
            total++;
        }

        return total;
    }
}