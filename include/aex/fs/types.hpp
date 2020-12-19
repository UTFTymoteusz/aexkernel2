#pragma once

#include "aex/mem/smartptr.hpp"

namespace AEX::FS {
    class Filesystem;
    class Mount;
    class ControlBlock;

    class INode;
    typedef Mem::SmartPointer<INode> INode_SP;

    class File;
    typedef Mem::SmartPointer<File> File_SP;

    class Directory;
    typedef Mem::SmartPointer<Directory> Directory_SP;
}