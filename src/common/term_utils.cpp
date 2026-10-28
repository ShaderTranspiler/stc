#include "common/term_utils.h"

#ifdef _WIN32
    #include <cstdlib>
    #include <external/io_windows.h>
#else
    #include <unistd.h>
#endif

namespace stc {

bool TerminalInfo::supports_color() {
    static bool inited    = false;
    static bool supported = false;

    if (inited)
        return supported;
    inited = true;

#ifdef _WIN32
    char* dupenv_buf{};
    errno_t dupenv_result = _dupenv_s(&dupenv_buf, nullptr, "NO_COLOR");

    if (dupenv_result != 0 || dupenv_buf != nullptr) {
        free(dupenv_buf);
        return false;
    }
#else
    if (std::getenv("NO_COLOR") != nullptr)
        return false;
#endif

#ifdef _WIN32
    if (_isatty(_fileno(stdout)) == 0 || _isatty(_fileno(stderr)) == 0)
        return false;

    HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE),
           stderr_handle = GetStdHandle(STD_ERROR_HANDLE);

    if (stdout_handle == INVALID_HANDLE_VALUE || stderr_handle == INVALID_HANDLE_VALUE)
        return false;

    DWORD stdout_mode = 0, stderr_mode = 0;

    if (GetConsoleMode(stdout_handle, &stdout_mode) == 0 ||
        GetConsoleMode(stderr_handle, &stderr_mode) == 0)
        return false;

    stdout_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT;
    stderr_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT;

    if (SetConsoleMode(stdout_handle, stdout_mode) == 0 ||
        SetConsoleMode(stderr_handle, stderr_mode) == 0)
        return false;

    supported = true;
#else
    supported = isatty(STDOUT_FILENO) != 0;
#endif

    return supported;
};

} // namespace stc
