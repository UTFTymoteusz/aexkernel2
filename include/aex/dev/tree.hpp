#pragma once

#include "aex/optional.hpp"
#include "aex/rcparray.hpp"
#include "aex/vector.hpp"

#include <stdint.h>

namespace AEX::Dev {
    class Device {
      public:
        struct attribute {
            enum type : uint8_t {
                INTEGER = 0,
                STRING  = 1,
                GROUP   = 2,
            };

            char name[16];
            type type;

            union {
                size_t            integer;
                char*             string;
                Vector<attribute> group;
            };

            attribute() {}

            attribute(const char* name, size_t integer) {
                strncpy(this->name, name, sizeof(this->name));

                type          = INTEGER;
                this->integer = integer;
            }

            attribute(const char* name, const char* string) {
                strncpy(this->name, name, sizeof(this->name));

                int len = strlen(string) + 1;

                type         = STRING;
                this->string = (char*) Heap::malloc(len);

                strncpy(this->string, string, len);
            }

            attribute(const char* name, Vector<attribute> group) {
                strncpy(this->name, name, sizeof(this->name));

                type        = GROUP;
                this->group = group;
            }
        };

        struct resource {
            enum type : uint8_t {
                MEMORY = 0,
                IO     = 1,
            };

            type type;

            union {
                size_t start;
                size_t value;
            };
            size_t end;

            resource() {}
        };

        char name[32];

        Device(const char* name);

        void                        addAttribute(attribute attribute);
        optional<Device::attribute> getAttribute(const char* name);

        void                       addResource(resource resource);
        optional<Device::resource> getResource(int index);

        void printAttributesDebug();
        void printResourcesDebug();

      private:
        Vector<attribute> _attributes;
        Vector<resource>  _resources;

        Spinlock _lock;
    };

    class Bus {
      public:
        char name[8];

        Bus(const char* name);

        void addDevice(Device* device);

      private:
        RCPArray<Device> _devices;
    };

    extern RCPArray<Bus> buses;
}