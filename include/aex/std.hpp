#pragma once

namespace std {
    class type_info {
        public:
        virtual ~type_info();

        const char* __type_name;
    };
}