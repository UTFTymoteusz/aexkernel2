#pragma once

#include "aex/fs/file.hpp"
#include "aex/fs/inode.hpp"
#include "aex/math.hpp"
#include "aex/mem/smartptr.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::FS {
    class INodeFile : public File {
        public:
        INodeFile(INode_SP inode) {
            _inode = inode;

            _block_size   = _inode->control_block->block_size;
            _cache_buffer = new uint8_t[_block_size];
        }

        ~INodeFile() {
            delete _cache_buffer;
        }

        optional<uint32_t> read(void* buffer, uint32_t count) {
            uint32_t requested_count = count;

            count = min<uint32_t>(_inode->size - _pos, count);
            if (count == 0)
                return 0;

            if (((uint64_t) _pos / _block_size) == _cached_block) {
                uint16_t offset = _pos & (_block_size - 1);
                uint16_t len    = min<uint32_t>((uint32_t) _block_size - offset, count);

                memcpy(buffer, _cache_buffer + offset, len);

                count -= len;
                _pos += len;

                buffer = (void*) ((uint8_t*) buffer + len);
            }

            if (count == 0)
                return requested_count;

            readBlocks(buffer, _pos, count);

            _pos += count;

            uint64_t last_block = (uint64_t) _pos / _block_size;
            if (last_block != _cached_block)
                readBlocks(_cache_buffer, last_block * _block_size, _block_size);

            _cached_block = last_block;

            return requested_count;
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
                new_pos = _inode->size + offset;
                break;
            default:
                break;
            }

            if (new_pos < 0 || (uint64_t) new_pos > _inode->size)
                return error_t::EINVAL;

            _pos = new_pos;
            return new_pos;
        }

        private:
        int64_t _pos = 0;

        uint16_t _block_size = 512;

        uint64_t _cached_block = 0xFFFFFFFFFFFFFFFF;
        uint8_t* _cache_buffer = nullptr;

        Mem::SmartPointer<INode> _inode;

        void readBlocks(void* buffer, uint64_t start, uint32_t len) {
            bool combo = false;

            uint64_t combo_start = 0;
            uint32_t combo_count = 0;

            uint8_t _overflow_buffer[_block_size];

            while (len > 0) {
                uint32_t offset = start - int_floor<uint64_t>(start, _block_size);
                uint32_t llen   = min(_block_size - offset, len);

                if (!isPerfectFit(start, llen)) {
                    if (combo) {
                        _inode->readBlocks(buffer, combo_start, combo_count);
                        buffer = (void*) ((uint8_t*) buffer + combo_count * _block_size);

                        combo = false;
                    }

                    _inode->readBlocks(_overflow_buffer, start / _block_size, 1);
                    memcpy(buffer, _overflow_buffer + offset, llen);

                    buffer = (void*) ((uint8_t*) buffer + llen);
                }
                else {
                    if (!combo) {
                        combo = true;

                        combo_start = start / _block_size;
                        combo_count = 0;
                    }

                    combo_count++;
                }

                start += llen;
                len -= llen;
            }

            if (combo) {
                _inode->readBlocks(buffer, combo_start, combo_count);
                buffer = (void*) ((uint8_t*) buffer + combo_count * _block_size);

                combo = false;
            }
        }

        bool isPerfectFit(uint64_t start, uint32_t len) {
            if (start % _block_size != 0)
                return false;

            if (len % _block_size != 0)
                return false;

            return true;
        }
    };
}