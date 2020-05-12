#include "aex/dev/tree/interface.hpp"

namespace AEX::Dev::Tree {
    Interface::~Interface() {}

    bool Interface::bind(Device*) {
        return false;
    }
}