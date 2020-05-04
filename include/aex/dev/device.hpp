#pragma once

#include "aex/optional.hpp"
#include "aex/spinlock.hpp"
#include "aex/string.hpp"
#include "aex/vector.hpp"

#include <stddef.h>
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
            enum type_t : uint8_t {
                MEMORY = 0,
                IO     = 1,
            };

            type_t type;

            union {
                size_t start;
                size_t value;
            };
            size_t end;

            resource() {}

            resource(type_t type, size_t value) {
                this->type  = type;
                this->value = value;
                this->end   = value;
            }

            resource(type_t type, size_t start, size_t end) {
                this->type  = type;
                this->start = start;
                this->end   = end;
            }
        };

        char name[32];
        char interface[24];

        void* interface_data;
        void* driver_data;

        Device(const char* name);

        void                        addAttribute(attribute attribute);
        optional<Device::attribute> getAttribute(const char* name);

        void                       addResource(resource resource);
        optional<Device::resource> getResource(int index);

        void printAttributesDebug();
        void printResourcesDebug();

        template <typename T>
        T getInterfaceData() {
            return (T) interface_data;
        }

        template <typename T>
        T getDriverData() {
            return (T) driver_data;
        }

      private:
        Vector<attribute> _attributes;
        Vector<resource>  _resources;

        Spinlock _lock;
    };
}