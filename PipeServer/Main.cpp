/* Based on https://learn.microsoft.com/en-us/windows/win32/ipc/multithreaded-pipe-server */
#include <windows.h> 
#include <stdio.h> 
#include <strsafe.h>
#include <vector>
#include "../PipeMessage.hpp"


DWORD ClientThread(void* lpvParam);

/** Windows pipe server sample. */
int main() {
    for (;;) {
        wprintf(L"Awaiting client connection on %s\n", PIPE_NAME);
        // Create named pipe
        HANDLE pipe = CreateNamedPipeW(
            PIPE_NAME,                // pipe name
            PIPE_ACCESS_DUPLEX,       // read/write access
            PIPE_TYPE_MESSAGE |       // message type pipe
            PIPE_READMODE_MESSAGE |   // message-read mode
            PIPE_WAIT,                // blocking mode
            PIPE_UNLIMITED_INSTANCES, // max. instances
            MAX_MESSAGE_SIZE,         // output buffer size
            MAX_MESSAGE_SIZE,         // input buffer size
            0,                        // client time-out
            NULL);                    // default security attribute
        if (pipe == INVALID_HANDLE_VALUE) {
            wprintf(L"CreateNamedPipe failed (err %d).\n", GetLastError());
            return -1;
        }

        // Wait for client to connect
        bool connected = ConnectNamedPipe(pipe, NULL);
        if (!connected) {
            if (GetLastError() == ERROR_PIPE_CONNECTED)
                connected = true; // client already conected before ConnectNamedPipe call
        }

        if (!connected) {
            // The client could not connect, so close the pipe.
            CloseHandle(pipe);

            // wait for next client connection
            continue;
        }

        printf("\nClient connected, creating a processing thread.\n");

        // Create a thread to handle client communication
        DWORD threadId = 0;
        unique_handle thread(CreateThread(
            NULL,            // no security attribute
            0,               // default stack size 
            ClientThread,    // thread proc
            pipe,            // thread parameter (transfer ownership)
            0,               // not suspended
            &threadId));

        if (thread == NULL) {
            wprintf(L"CreateThread failed (err %d).\n", GetLastError());
            return -1;
        }

        pipe = NULL; // closed in ClientThread function

        // wait for next client connection
    }

    return 0;
}

DWORD ClientThread(void* threadParam) {
    HANDLE pipe = (HANDLE)threadParam; // acquire ownership

    // Loop until done reading
    for (;;) {
        // Read client requests from the pipe
        std::vector<char> requestBuf(MAX_MESSAGE_SIZE);
        DWORD bytesRead = 0;
        BOOL success = ReadFile(pipe,
            requestBuf.data(), // buffer to receive data 
            (DWORD)requestBuf.size(), // size of buffer 
            &bytesRead, // number of bytes read 
            NULL);        // blocking call
        if (!success || (bytesRead == 0)) {
            if (GetLastError() == ERROR_BROKEN_PIPE) {
                wprintf(L"Client disconnected.\n");
            } else {
                wprintf(L"ReadFile failed (err %d).\n", GetLastError());
            }
            break;
        }

        // process incoming message
        wprintf(L"Client Request: %hs\n", requestBuf.data());

        // copy reply to output buffer
        std::vector<char> replyBuf(MAX_MESSAGE_SIZE);
        HRESULT hr = StringCchCopyA(replyBuf.data(), MAX_MESSAGE_SIZE, "Answer from server");
        if (FAILED(hr)) {
            replyBuf.clear();
            printf("ERROR: Output buffer too small.\n");
            return 1;
        }
        replyBuf.resize((strlen(replyBuf.data()) + 1)); // add zero termination

        // write reply to pipe
        DWORD bytesWritten = 0;
        success = WriteFile(pipe,
            replyBuf.data(), // buffer to write from 
            (DWORD)replyBuf.size(), // number of bytes to write 
            &bytesWritten,   // number of bytes written 
            NULL);        // blocking call
        if (!success || (replyBuf.size() != bytesWritten)) {
            wprintf(L"WriteFile failed (err %d).\n", GetLastError());
            break;
        }
    }

    // flush & disconnect pipe before closing it
    FlushFileBuffers(pipe);
    DisconnectNamedPipe(pipe);
    CloseHandle(pipe);
    return 0;
}
