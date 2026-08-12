#include <memory>
#include <string_view>


struct ProtocolMessage {
    static constexpr wchar_t PIPE_NAME[] = L"\\\\.\\pipe\\mynamedpipe";

    static constexpr size_t MAX_MESSAGE_SIZE = 510; // in bytes

    uint16_t length = 0; // incl. this field
    char message[MAX_MESSAGE_SIZE]{}; // actual lenght specified by "length" (not null-terminated)

    std::string_view Get() const {
        return {message, (length - sizeof(length))};
    }

    void Set(std::string_view msg) {
        length = sizeof(length) + (uint16_t)msg.size();
        memcpy(message, msg.data(), msg.size()); // not null-terminated
    }
};


struct HandleDeleter {
    using pointer = HANDLE;

    void operator()(HANDLE handle) const {
        if (handle == nullptr) // for default-constructred unique_ptr
            return;

        if (handle == INVALID_HANDLE_VALUE)
            return;

        CloseHandle(handle);
    }
};

using unique_handle = std::unique_ptr<void, HandleDeleter>;
