#include "aex/dev/tree.hpp"

#include "aex/mem.hpp"
#include "aex/optional.hpp"
#include "aex/printk.hpp"
#include "aex/spinlock.hpp"
#include "aex/string.hpp"

namespace AEX::Dev::Tree {
    extern void register_base_drivers();

    Mem::SmartArray<Bus> buses;
    Tree::Device*        root_device = nullptr;
    Spinlock             lock;

    void init() {
        Tree::register_base_drivers();

        root_device = new Tree::Device("root", nullptr);
    }

    bool register_device(const char* bus_name, Device* device) {
        for (auto iterator = buses.getIterator(); auto bus = iterator.next();) {
            if (strcmp(bus->name, bus_name) != 0)
                continue;

            bus->registerDevice(device);
            return true;
        }

        return false;
    }

    bool register_driver(const char* bus_name, Driver* driver) {
        for (auto iterator = buses.getIterator(); auto bus = iterator.next();) {
            if (strcmp(bus->name, bus_name) != 0)
                continue;

            bus->registerDriver(driver);

            return true;
        }

        return false;
    }

    Mem::SmartPointer<Bus> get_bus(const char* bus_name) {
        int index = -1;

        for (auto iterator = buses.getIterator(); auto bus = iterator.next();) {
            if (strcmp(bus->name, bus_name) != 0)
                continue;

            index = iterator.index();
            break;
        }

        if (index == -1)
            return buses.get(-1);

        return buses.get(index);
    }

    bool bus_exists(const char* bus_name) {
        for (auto iterator = buses.getIterator(); auto bus = iterator.next();) {
            if (strcmp(bus->name, bus_name) != 0)
                continue;

            return true;
        }

        return false;
    }

    void walk_device(Tree::Device* device, int depth) {
        for (int i = 0; i < depth; i++)
            printk("  ");

        printk("%s\n", device->name);

        for (auto iterator = device->children.getIterator(); auto child = iterator.next();)
            walk_device(child, depth + 1);
    }

    void walk_bus(Tree::Bus* bus) {
        printk("%s\n", bus->name);
        printk("  Devices\n");

        for (auto iterator = bus->devices.getIterator(); auto device = iterator.next();)
            printk("    %s\n", device->name);

        printk("  Drivers\n");

        for (auto iterator = bus->drivers.getIterator(); auto driver = iterator.next();)
            printk("    %s\n", driver->name);
    }

    void print_debug() {
        printk("Device Tree\n");
        walk_device(root_device, 0);
        printk("\n");

        printk("Buses\n");
        for (auto iterator = buses.getIterator(); auto bus = iterator.next();)
            walk_bus(bus);

        printk("\n");
    }
}