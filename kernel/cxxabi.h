#pragma once

#include "aex/std.hpp"

#include <stdint.h>

namespace __cxxabiv1 {
    class __class_type_info;

    struct __base_class_type_info {
        public:
        const __class_type_info* __base_type;
        int64_t                  __offset_flags;

        enum __offset_flags_masks { __virtual_mask = 0x1, __public_mask = 0x2, __offset_shift = 8 };
    };

    class __class_type_info : public std::type_info {
        public:
        virtual ~__class_type_info();
    };

    class __si_class_type_info : public __class_type_info {
        public:
        virtual ~__si_class_type_info();

        const __class_type_info* __base_type;
    };

    class __vmi_class_type_info : public __class_type_info {
        public:
        virtual ~__vmi_class_type_info();

        uint32_t               __flags;
        uint32_t               __base_count;
        __base_class_type_info __base_info[1];

        enum __flags_masks { __non_diamond_repeat_mask = 0x1, __diamond_shaped_mask = 0x2 };
    };
}

namespace abi = __cxxabiv1;