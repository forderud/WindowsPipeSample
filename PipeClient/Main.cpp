/* Based on https://learn.microsoft.com/en-us/windows/win32/ipc/named-pipe-client */
#include <windows.h> 
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <string>
#include <vector>
#include "../PipeMessage.hpp"


/** Windows pipe client sample. */
int main(int argc, char* argv[]) {
    std::string message = "Some message from client.";
    if (argc > 1)
        message = argv[1];

    // Try to open a named pipe; wait for it, if necessary. 
    unique_handle pipe;
    for (;;) {
        pipe.reset(CreateFileW(
            PIPE_NAME,      // pipe name
            GENERIC_READ |  // read and write access
            GENERIC_WRITE,
            0,              // no sharing
            NULL,           // default security attributes
            OPEN_EXISTING,  // opens existing pipe
            0,              // default attributes
            NULL));         // no template file

        if (pipe.get() != INVALID_HANDLE_VALUE)
            break; // valid handle

        // Exit if other error than ERROR_PIPE_BUSY
        if (GetLastError() != ERROR_PIPE_BUSY) {
            wprintf(L"Could not open pipe (err %d)\n", GetLastError());
            return -1;
        }
        // pipe is busy, so wait for 10 seconds before retrying
        if (!WaitNamedPipeW(PIPE_NAME, 10000)) {
            printf("Could not open pipe: 10 second wait timed out.");
            return -1;
        }

        // retry connecting to busy pipe
        continue;
    }

    wprintf(L"Connected to pipe %s\n\n", PIPE_NAME);

    // The pipe connected; change to message-read mode. 
    DWORD mode = PIPE_READMODE_MESSAGE;
    BOOL success = SetNamedPipeHandleState(pipe.get(),
        &mode,    // new pipe mode 
        NULL,     // don't set maximum bytes 
        NULL);    // don't set maximum time 
    if (!success) {
        wprintf(L"SetNamedPipeHandleState failed (err %d)\n", GetLastError());
        return -1;
    }

    wprintf(L"Sending message: %hs\n", message.c_str());

    DWORD bytesWritten = 0;
    success = WriteFile(pipe.get(),
        message.c_str(),// message 
        (DWORD)(message.length() + 1), // message length
        &bytesWritten,   // bytes written
        NULL);           // blocking call
    if (!success) {
        wprintf(L"WriteFile to pipe failed (err %d)\n", GetLastError());
        return -1;
    }

    do {
        // Read from pipe
        std::vector<char> replyBuf(MAX_MESSAGE_SIZE);
        DWORD bytesRead = 0;
        success = ReadFile(pipe.get(),
            replyBuf.data(), // buffer to receive reply
            (DWORD)replyBuf.size(), // size of buffer
            &bytesRead,// number of bytes read
            NULL);    // blocking call
        if (!success && (GetLastError() != ERROR_MORE_DATA))
            break;

        wprintf(L"Received message: %hs\n", replyBuf.data());
    } while (!success);  // repeat loop if ERROR_MORE_DATA

    if (!success) {
        wprintf(L"ReadFile from pipe failed (err %d)\n", GetLastError());
        return -1;
    }

    return 0;
}
