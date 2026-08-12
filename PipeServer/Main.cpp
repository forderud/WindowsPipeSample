/* Based on https://learn.microsoft.com/en-us/windows/win32/ipc/multithreaded-pipe-server */
#include <windows.h> 
#include <stdio.h> 
#include <strsafe.h>
#include "../PipeMessage.hpp"


DWORD ClientThread(void* lpvParam);

/** Windows pipe server sample. */
int main() {
    for (;;) {
        wprintf(L"Awaiting client connection on %s\n", ProtocolMessage::PIPE_NAME);
        // Create named pipe
        unique_handle pipe(CreateNamedPipeW(
            ProtocolMessage::PIPE_NAME,       // pipe name
            PIPE_ACCESS_DUPLEX,       // read/write access
            PIPE_TYPE_MESSAGE |       // message type pipe
            PIPE_READMODE_MESSAGE |   // message-read mode
            PIPE_WAIT,                // blocking mode
            PIPE_UNLIMITED_INSTANCES, // max. instances
            sizeof(ProtocolMessage),  // output buffer size
            sizeof(ProtocolMessage),  // input buffer size
            0,                        // client time-out
            NULL));                   // default security attribute
        if (pipe.get() == INVALID_HANDLE_VALUE) {
            wprintf(L"CreateNamedPipe failed (err %d).\n", GetLastError());
            return -1;
        }

        // Wait for client to connect
        bool connected = ConnectNamedPipe(pipe.get(), NULL);
        if (!connected) {
            if (GetLastError() == ERROR_PIPE_CONNECTED)
                connected = true; // client already conected before ConnectNamedPipe call
        }

        if (!connected) {
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
            pipe.get(),      // thread parameter (transfer ownership)
            0,               // not suspended
            &threadId));

        if (thread == NULL) {
            wprintf(L"CreateThread failed (err %d).\n", GetLastError());
            return -1;
        }

        pipe.release(); // ownership transferred to thread

        // wait for next client connection
        continue;
    }

    return 0;
}

DWORD ClientThread(void* threadParam) {
    unique_handle pipe((HANDLE)threadParam); // acquire ownership

    // Loop until done reading
    for (;;) {
        // Read client requests from the pipe
        ProtocolMessage requestBuf{};
        DWORD bytesRead = 0;
        BOOL success = ReadFile(pipe.get(),
            &requestBuf, // buffer to receive data 
            (DWORD)sizeof(requestBuf), // size of buffer 
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
        wprintf(L"Client Request: ");
        requestBuf.Print();
        wprintf(L"\n");

        // copy reply to output buffer
        ProtocolMessage replyBuf{};
        replyBuf.Set("Answer from server");

        // write reply to pipe
        DWORD bytesWritten = 0;
        success = WriteFile(pipe.get(),
            &replyBuf, // buffer to write from 
            replyBuf.Size(), // number of bytes to write 
            &bytesWritten,   // number of bytes written 
            NULL);        // blocking call
        if (!success || (replyBuf.Size() != bytesWritten)) {
            wprintf(L"WriteFile failed (err %d).\n", GetLastError());
            break;
        }
    }

    // flush & disconnect pipe before closing it
    FlushFileBuffers(pipe.get());
    DisconnectNamedPipe(pipe.get());
    return 0;
}
