#include <memory>

#define MAX_MESSAGE_SIZE 512 // in bytes

const wchar_t PIPE_NAME[] = L"\\\\.\\pipe\\mynamedpipe";

// Protocol:
// Bidirectional communication of null-terminated ASCII strings.



struct HandleDeleter {
    using pointer = HANDLE;

    void operator()(HANDLE handle) const {
        if ((handle != INVALID_HANDLE_VALUE) && (handle != nullptr)) {
            CloseHandle(handle);
        }
    }
};

using unique_handle = std::unique_ptr<void, HandleDeleter>;
