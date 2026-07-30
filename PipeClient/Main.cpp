/* Based on https://learn.microsoft.com/en-us/windows/win32/ipc/named-pipe-client */
#include <windows.h> 
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <string>
#include "../PipeMessage.hpp"


int main(int argc, char* argv[]) {
    std::string message = "Default message from client.";
    if (argc > 1)
        message = argv[1];

    // Try to open a named pipe; wait for it, if necessary. 
    HANDLE pipe = nullptr;
    while (1) {
        pipe = CreateFile(
            PIPE_NAME,      // pipe name 
            GENERIC_READ |  // read and write access 
            GENERIC_WRITE,
            0,              // no sharing 
            NULL,           // default security attributes
            OPEN_EXISTING,  // opens existing pipe 
            0,              // default attributes 
            NULL);          // no template file 

        if (pipe != INVALID_HANDLE_VALUE)
            break; // valid handle

        // Exit if other error than ERROR_PIPE_BUSY
        if (GetLastError() != ERROR_PIPE_BUSY) {
            wprintf(L"Could not open pipe (err %d)\n", GetLastError());
            return -1;
        }
        // pipe is busy, so wait for 20 seconds before retrying
        if (!WaitNamedPipe(PIPE_NAME, 20000)) {
            printf("Could not open pipe: 20 second wait timed out.");
            return -1;
        }

        // retry connecting to busy pipe
        continue;
    }

    // The pipe connected; change to message-read mode. 
    DWORD mode = PIPE_READMODE_MESSAGE;
    BOOL fSuccess = SetNamedPipeHandleState(
        pipe,    // pipe handle 
        &mode,    // new pipe mode 
        NULL,     // don't set maximum bytes 
        NULL);    // don't set maximum time 
    if (!fSuccess) {
        wprintf(L"SetNamedPipeHandleState failed (err %d)\n", GetLastError());
        return -1;
    }

    wprintf(L"Sending message: %hs\n", message.c_str());

    DWORD cbWritten = 0;
    fSuccess = WriteFile(
        pipe,
        message.c_str(),// message 
        (DWORD)(message.length() + 1), // message length 
        &cbWritten,      // bytes written 
        NULL);           // not overlapped 

    if (!fSuccess) {
        wprintf(L"WriteFile to pipe failed (err %d)\n", GetLastError());
        return -1;
    }

    do {
        // Read from the pipe.
        char replyBuf[BUF_SIZE];
        DWORD  cbRead = 0;
        fSuccess = ReadFile(
            pipe,
            replyBuf, // buffer to receive reply 
            BUF_SIZE, // size of buffer 
            &cbRead,  // number of bytes read 
            NULL);    // not overlapped 

        if (!fSuccess && GetLastError() != ERROR_MORE_DATA)
            break;

        wprintf(L"Reply: %hs\n", replyBuf);
    } while (!fSuccess);  // repeat loop if ERROR_MORE_DATA 

    if (!fSuccess) {
        wprintf(L"ReadFile from pipe failed (err %d)\n", GetLastError());
        return -1;
    }

    wprintf(L"\nEnd of message.\n");

    CloseHandle(pipe);

    return 0;
}
