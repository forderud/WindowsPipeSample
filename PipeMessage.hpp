
#define MAX_MESSAGE_SIZE 512 // in bytes

const wchar_t PIPE_NAME[] = L"\\\\.\\pipe\\mynamedpipe";

// Protocol:
// Bidirectional communication of null-terminated ASCII strings.
