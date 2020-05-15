#pragma once

#include "aex/proc/thread.hpp"

namespace AEX::IPC {
    class MessageQueue {
      public:
        MessageQueue();

      private:
        void pushMessage(void* data, int len);
    };
}