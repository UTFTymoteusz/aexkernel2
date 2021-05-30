#include "usr.hpp"

#include "aex/proc.hpp"

using namespace AEX;

bool copy_and_canonize(char buffer[FS::PATH_MAX], const usr_char* usr_path) {
    auto current    = Proc::Process::current();
    auto strlen_try = usr_strlen(usr_path);
    if (!strlen_try)
        return false;

    size_t len = min<size_t>(strlen_try.value + 1, FS::PATH_MAX);
    if (!u2k_memcpy(buffer, usr_path, len + 1))
        return false;

    buffer[len] = '\0';

    if (buffer[0] != '/') {
        char buffer_b[FS::PATH_MAX];
        FS::canonize_path(buffer, current->get_cwd(), buffer_b, FS::PATH_MAX);

        memcpy(buffer, buffer_b, FS::PATH_MAX);
    }

    return true;
}
