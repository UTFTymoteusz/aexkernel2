#pragma once

namespace AEX::FS {
    class File {
      public:
        virtual ~File();

        static File open(char* path);

      private:
    };
}