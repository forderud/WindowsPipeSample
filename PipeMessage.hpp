#include <memory>
#include <string_view>


class ProtocolMessage {
public:
    static constexpr wchar_t PIPE_NAME[] = L"\\\\.\\pipe\\mynamedpipe";

private:
    uint16_t length = 0; // Total message size, including "length" field
    char message[510]{}; // ASCII string (not null-terminated). Actual lenght specified by "length"

public:
    DWORD Size() const {
        return length;
    }

    std::string_view Get() const {
        return {message, (length - sizeof(length))};
    }

    void Set(std::string_view msg) {
        length = sizeof(length) + (uint16_t)msg.size();
        memcpy(message, msg.data(), msg.size()); // not null-terminated
    }

    void Print() const {
        auto msg = Get();
        wprintf(L"%.*hs", (int)msg.length(), msg.data());
    }
};
static_assert(sizeof(ProtocolMessage) == 512);


struct HandleDeleter {
    using pointer = HANDLE;

    void operator()(HANDLE handle) const {
        if (handle == NULL)
            return; // invalid thread handle

        if (handle == INVALID_HANDLE_VALUE)
            return; // invalid file, pipe handle

        CloseHandle(handle);
    }
};

using unique_handle = std::unique_ptr<void, HandleDeleter>;
