#include "devfs.hpp"

#include "aex/dev/blockdevice.hpp"
#include "aex/dev/dev.hpp"
#include "aex/dev/device.hpp"
#include "aex/printk.hpp"
#include "aex/string.hpp"

namespace AEX::FS {
    class DevFSDirectory : public File {
        public:
        DevFSDirectory() : File() {}

        optional<dir_entry> readdir() {
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

    optional<file_info> DevFSMount::info(const char* lpath) {
        printk("devfs: info(%s) called\n", lpath);
        // make this work properly for /

        lpath++;
        if (*lpath == '\0')
            return ENOENT;

        for (auto iterator = Dev::devices.getIterator(); auto device = iterator.next();) {
            if (strcmp(device->name, lpath) != 0)
                continue;

            auto info = file_info();

            switch (device->type) {
            case Dev::Device::type_t::BLOCK:
                info.type = type_t::BLOCK;
                break;
            case Dev::Device::type_t::CHAR:
                info.type = type_t::CHAR;
                break;
            case Dev::Device::type_t::NET:
                info.type = type_t::NET;
                break;
            }

            info.dev_id = iterator.index();

            return info;
        }

        return ENOENT;
    }
}