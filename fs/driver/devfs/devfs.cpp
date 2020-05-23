#include "devfs.hpp"

#include "aex/dev/block.hpp"
#include "aex/dev/dev.hpp"
#include "aex/printk.hpp"
#include "aex/string.hpp"

namespace AEX::FS {
    class DevFSDirectory : public File {
      public:
        DevFSDirectory() : File() {}

        optional<File::dir_entry> readdir() {
            auto device = _iterator.next();
            if (!device)
                return {};

            return dir_entry(device->name);
        }

      private:
        Mem::SmartArray<Dev::Device>::Iterator _iterator = Dev::devices.getIterator();
    };

    void DevFS::init() {
        new DevFS();
    }

    DevFS::DevFS() : Filesystem("devfs") {}
    DevFS::~DevFS() {}

    optional<Mount*> DevFS::mount(const char* source) {
        if (source)
            return {};

        return new DevFSMount();
    }

    optional<Mem::SmartPointer<File>> DevFSMount::opendir(const char* lpath) {
        printk("devfs: opendir(%s) called\n", lpath);

        if (strcmp(lpath, "/") == 0) {
            auto dir = new DevFSDirectory();

            return Mem::SmartPointer<File>(dir);
        }

        return ENOENT;
    }
}