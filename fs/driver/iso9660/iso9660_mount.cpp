#include "iso9660_mount.hpp"

#include "aex/fs/path.hpp"
#include "aex/math.hpp"
#include "aex/proc/thread.hpp"
#include "aex/string.hpp"

#include "types.hpp"


namespace AEX::FS {
    void clean_name(char* buffer);

    class ISO9660File : public File {
      public:
        ISO9660File(Mem::SmartPointer<Dev::Block> block, const iso9660_dentry& dentry) : File() {
            _dentry    = dentry;
            _block_dev = block;
        }

        optional<uint32_t> read(void* buf, uint32_t count) {
            count = min(_dentry.data_len.le - _pos, count);
            if (count == 0)
                return 0;

            _block_dev->read((uint8_t*) buf,
                             (uint64_t) _dentry.data_lba.le * BLOCK_SIZE + (uint64_t) _pos, count);

            _pos += count;

            return count;
        }

        optional<dir_entry> readdir() {
            return error_t::ENOTDIR;
        }

        optional<int64_t> seek(int64_t offset, seek_mode mode) {
            int64_t new_pos = 0;

            switch (mode) {
            case seek_mode::SET:
                new_pos = offset;
                break;
            case seek_mode::CURRENT:
                new_pos = _pos + offset;
                break;
            case seek_mode::END:
                new_pos = _dentry.data_len.le + offset;
                break;
            default:
                break;
            }

            if (new_pos < 0 || new_pos > _dentry.data_len.le)
                return error_t::EINVAL;

            _pos = new_pos;
            return new_pos;
        }

      private:
        int64_t _pos = 0;

        uint64_t _cached_sector = 0xFFFFFFFFFFFFFFFF;
        uint8_t  _buffer[BLOCK_SIZE];

        Mem::SmartPointer<Dev::Block> _block_dev;

        iso9660_dentry _dentry;
    };

    class ISO9660Directory : public File {
      public:
        ISO9660Directory(Mem::SmartPointer<Dev::Block> block, const iso9660_dentry& dentry)
            : File() {
            _dentry    = dentry;
            _block_dev = block;

            _block_dev->read(_buffer, _dentry.data_lba.le * BLOCK_SIZE + _pos, BLOCK_SIZE);
        }

        optional<uint32_t> read() {
            return error_t::EISDIR;
        }

        optional<dir_entry> readdir() {
            if (_pos >= _dentry.data_len.le)
                return {};

            char name_buffer[Path::MAX_FILENAME_LEN];
            name_buffer[Path::MAX_FILENAME_LEN - 1] = '\0';

            while (_pos < _dentry.data_len.le) {
                auto ldentry = (iso9660_dentry*) &_buffer[_pos];

                uint8_t len = ldentry->len;
                if (len == 0) {
                    _pos = ((_pos + BLOCK_SIZE) / BLOCK_SIZE) * BLOCK_SIZE;
                    if (_pos >= _dentry.data_len.le)
                        return {};

                    _block_dev->read(_buffer, _dentry.data_lba.le * BLOCK_SIZE + _pos, BLOCK_SIZE);

                    continue;
                }

                _pos += len;

                if (ldentry->name_len == 1 &&
                    (ldentry->name[0] == '\0' || ldentry->name[0] == '\1'))
                    continue;

                memcpy(name_buffer, ldentry->name, min(sizeof(name_buffer) - 1, ldentry->name_len));
                name_buffer[ldentry->name_len] = '\0';

                clean_name(name_buffer);

                // I'm too lazy atm
                int lpos = sizeof(iso9660_dentry) + ldentry->name_len;
                lpos += lpos % 2;

                int max_len = 255 - lpos;

                while (lpos < max_len) {
                    int susp_pos = _pos - len + lpos;

                    if (_buffer[susp_pos] < 'A' || _buffer[susp_pos] > 'Z')
                        break;

                    lpos += _buffer[susp_pos + 2];

                    if (memcmp(&_buffer[susp_pos], "CE", 2) == 0)
                        kpanic("iso9660: CE encountered");
                    else if (memcmp(&_buffer[susp_pos], "NM", 2) == 0) {
                        strncpy(name_buffer, (const char*) &_buffer[susp_pos + 5],
                                min(_buffer[susp_pos + 2] - 5 + 1, 255));
                    }
                }

                auto dentry_ret = dir_entry(name_buffer);

                dentry_ret.type = ldentry->isDirectory() ? type_t::DIRECTORY : type_t::REGULAR;

                return dentry_ret;
            }

            return {};
        }

      private:
        uint8_t _buffer[2048];
        int64_t _pos = 0;

        Mem::SmartPointer<Dev::Block> _block_dev;

        iso9660_dentry _dentry;
    };

    optional<Mem::SmartPointer<File>> ISO9660Mount::open(const char* lpath) {
        auto dentry_try = findDentry(lpath);
        if (!dentry_try.has_value)
            return dentry_try.error_code;

        auto dentry = dentry_try.value;
        if (dentry.isDirectory())
            return error_t::EISDIR;

        return Mem::SmartPointer<File>(new ISO9660File(_block_dev, dentry));
    }

    optional<Mem::SmartPointer<File>> ISO9660Mount::opendir(const char* lpath) {
        auto dentry_try = findDentry(lpath);
        if (!dentry_try.has_value)
            return dentry_try.error_code;

        auto dentry = dentry_try.value;
        if (!dentry.isDirectory())
            return error_t::ENOTDIR;

        return Mem::SmartPointer<File>(new ISO9660Directory(_block_dev, dentry));
    }

    optional<file_info> ISO9660Mount::info(const char* lpath) {
        printk("iso9660: info(%s)\n", lpath);

        auto dentry_try = findDentry(lpath);
        if (!dentry_try.has_value)
            return dentry_try.error_code;

        auto dentry = dentry_try.value;
        auto info   = file_info();

        info.type              = dentry.isDirectory() ? type_t::DIRECTORY : type_t::REGULAR;
        info.total_size        = dentry.data_len.le;
        info.containing_dev_id = _block_dev->id;

        return info;
    }

    optional<iso9660_dentry> ISO9660Mount::findDentry(const char* lpath) {
        iso9660_dentry dentry = _root_dentry;

        uint8_t buffer[BLOCK_SIZE];
        char    name_buffer[Path::MAX_FILENAME_LEN];

        name_buffer[Path::MAX_FILENAME_LEN - 1] = '\0';

        // this code is a mess, help
        for (auto walker = Path::Walker(lpath); auto piece = walker.next();) {
            _block_dev->read(buffer, dentry.data_lba.le * BLOCK_SIZE, BLOCK_SIZE);

            bool found = false;

            for (uint32_t pos = 0; pos < dentry.data_len.le;) {
                uint32_t floored = (pos / BLOCK_SIZE) * BLOCK_SIZE;
                auto     ldentry = (iso9660_dentry*) &buffer[pos - floored];

                uint8_t len = ldentry->len;
                if (len == 0) {
                    pos = ((pos + BLOCK_SIZE) / BLOCK_SIZE) * BLOCK_SIZE;
                    continue;
                }

                pos += len;

                if (ldentry->name_len == 1 &&
                    (ldentry->name[0] == '\0' || ldentry->name[0] == '\1'))
                    continue;

                memcpy(name_buffer, ldentry->name, min(sizeof(name_buffer) - 1, ldentry->name_len));
                name_buffer[ldentry->name_len] = '\0';

                clean_name(name_buffer);

                // I'm too lazy atm
                int lpos = sizeof(iso9660_dentry) + ldentry->name_len;
                lpos += lpos % 2;

                int max_len = 255 - lpos;

                while (lpos < max_len) {
                    int susp_pos = pos - len + lpos;

                    if (buffer[susp_pos] < 'A' || buffer[susp_pos] > 'Z')
                        break;

                    lpos += buffer[susp_pos + 2];

                    if (memcmp(&buffer[susp_pos], "CE", 2) == 0)
                        kpanic("iso9660: CE encountered");
                    else if (memcmp(&buffer[susp_pos], "NM", 2) == 0) {
                        strncpy(name_buffer, (const char*) &buffer[susp_pos + 5],
                                min(buffer[susp_pos + 2] - 5 + 1, 255));
                    }
                }

                if (strcmp(name_buffer, piece) == 0) {
                    if (!walker.isFinal() && !ldentry->isDirectory())
                        return error_t::ENOTDIR;

                    dentry = *ldentry;
                    found  = true;

                    break;
                }
            }

            if (!found)
                return error_t::ENOENT;
        }

        return dentry;
    }

    void clean_name(char* buffer) {
        for (int i = 0; i < Path::MAX_FILENAME_LEN; i++) {
            if (buffer[i] == ';')
                buffer[i] = '\0';

            // buffer[i] = tolower(buffer[i]);
        }
    }
}