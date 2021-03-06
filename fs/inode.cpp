#include "aex/fs/inode.hpp"

namespace AEX::FS {
    INode::~INode() {}

    error_t INode::readBlocks(void*, uint64_t, uint16_t) {
        return ENOSYS;
    }

    error_t INode::writeBlocks(const void*, uint64_t, uint16_t) {
        return ENOSYS;
    }

    error_t INode::update() {
        return ENOSYS;
    }

    optional<dir_entry> INode::readDir(dir_context*) {
        return ENOSYS;
    }

    error_t INode::seekDir(dir_context*, long) {
        return ENOSYS;
    }

    long INode::tellDir(dir_context*) {
        return -1;
    }
}