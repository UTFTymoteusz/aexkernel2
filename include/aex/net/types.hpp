#pragma once

#include "aex/mem/smartptr.hpp"

namespace AEX::Net {
    class Socket;
    typedef Mem::SmartPointer<Socket> Socket_SP;

}