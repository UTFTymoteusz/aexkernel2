#include "aex/fs/inode.hpp"

namespace AEX::FS {
    INode::~INode() {}

    error_t INode::read(void*, blk_t, blkcnt_t) {
        return ENOSYS;
    }

    error_t INode::write(const void*, blk_t, blkcnt_t) {
        return ENOSYS;
    }

    error_t INode::truncate(size_t, bool) {
        return ENOSYS;
    }

    error_t INode::update() {
        return ENOSYS;
    }

    error_t INode::purge() {
        return ENOSYS;
    }

    optional<INode_SP> INode::creat(const char*, mode_t, fs_type_t) {
        return ENOSYS;
    }

    optional<dirent> INode::readdir(dir_context*) {
        return ENOSYS;
    }

    error_t INode::seekdir(dir_context*, long) {
        return ENOSYS;
    }

    long INode::telldir(dir_context*) {
        return -1;
    }

    error_t INode::remove(const char*) {
        return ENOSYS;
    }
}