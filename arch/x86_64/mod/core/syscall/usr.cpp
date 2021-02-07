#include "usr.hpp"

#include "aex/proc.hpp"

using namespace AEX;

bool copy_and_canonize(char buffer[FS::MAX_PATH_LEN], const usr_char* usr_path) {
    auto current    = Proc::Process::current();
    auto strlen_try = usr_strlen(usr_path);
    if (!strlen_try)
        return false;

    int len = min<int>(strlen_try.value + 1, FS::MAX_PATH_LEN);
    if (!u2k_memcpy(buffer, usr_path, len + 1))
        return false;

    buffer[len] = '\0';

    if (buffer[0] != '/') {
        char buffer_b[FS::MAX_PATH_LEN];
        FS::canonize_path(buffer, current->get_cwd(), buffer_b, FS::MAX_PATH_LEN);

        memcpy(buffer, buffer_b, FS::MAX_PATH_LEN);
    }

    return true;
}
