/* Based on https://learn.microsoft.com/en-us/windows/win32/ipc/named-pipe-client */
#include <windows.h> 
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include "../PipeMessage.hpp"


int wmain(int argc, wchar_t* argv[]) {
    const BUF_TYPE* lpvMessage = L"Default message from client.";
    if (argc > 1)
        lpvMessage = argv[1];

    // Try to open a named pipe; wait for it, if necessary. 
    HANDLE hPipe = nullptr;
    while (1) {
        hPipe = CreateFile(
            PIPE_NAME,      // pipe name 
            GENERIC_READ |  // read and write access 
            GENERIC_WRITE,
            0,              // no sharing 
            NULL,           // default security attributes
            OPEN_EXISTING,  // opens existing pipe 
            0,              // default attributes 
            NULL);          // no template file 

        if (hPipe != INVALID_HANDLE_VALUE)
            break; // valid handle

        // Exit if other error than ERROR_PIPE_BUSY
        if (GetLastError() != ERROR_PIPE_BUSY) {
            wprintf(L"Could not open pipe. GLE=%d\n", GetLastError());
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
    DWORD dwMode = PIPE_READMODE_MESSAGE;
    BOOL fSuccess = SetNamedPipeHandleState(
        hPipe,    // pipe handle 
        &dwMode,  // new pipe mode 
        NULL,     // don't set maximum bytes 
        NULL);    // don't set maximum time 
    if (!fSuccess) {
        wprintf(L"SetNamedPipeHandleState failed. GLE=%d\n", GetLastError());
        return -1;
    }

    // Send a message to the pipe server. 
    DWORD cbToWrite = (lstrlen(lpvMessage) + 1)*sizeof(BUF_TYPE);
    wprintf(L"Sending %d byte message: \"%s\"\n", cbToWrite, lpvMessage);

    DWORD cbWritten = 0;
    fSuccess = WriteFile(
        hPipe,           // pipe handle 
        lpvMessage,      // message 
        cbToWrite,       // message length 
        &cbWritten,      // bytes written 
        NULL);           // not overlapped 

    if (!fSuccess) {
        wprintf(L"WriteFile to pipe failed. GLE=%d\n", GetLastError());
        return -1;
    }

    printf("\nMessage sent to server, receiving reply as follows:\n");

    do {
        // Read from the pipe.
        BUF_TYPE chBuf[BUF_SIZE];
        DWORD  cbRead = 0;
        fSuccess = ReadFile(
            hPipe,    // pipe handle 
            chBuf,    // buffer to receive reply 
            BUF_SIZE*sizeof(BUF_TYPE), // size of buffer 
            &cbRead,  // number of bytes read 
            NULL);    // not overlapped 

        if (!fSuccess && GetLastError() != ERROR_MORE_DATA)
            break;

        wprintf(L"\"%s\"\n", chBuf);
    } while (!fSuccess);  // repeat loop if ERROR_MORE_DATA 

    if (!fSuccess) {
        wprintf(L"ReadFile from pipe failed. GLE=%d\n", GetLastError());
        return -1;
    }

    wprintf(L"\nEnd of message.\n");

    CloseHandle(hPipe);

    return 0;
}
