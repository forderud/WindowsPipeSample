/* Based on https://learn.microsoft.com/en-us/windows/win32/ipc/multithreaded-pipe-server */
#include <windows.h> 
#include <stdio.h> 
#include <strsafe.h>
#include <vector>
#include "../PipeMessage.hpp"


DWORD WINAPI InstanceThread(void* lpvParam);

int main() {
    // The main loop creates an instance of the named pipe and 
    // then waits for a client to connect to it. When the client 
    // connects, a thread is created to handle communications 
    // with that client, and this loop is free to wait for the
    // next client connect request. It is an infinite loop.
    for (;;) {
        wprintf(L"\nPipe Server: Main thread awaiting client connection on %s\n", PIPE_NAME);
        HANDLE pipe = CreateNamedPipeW(
            PIPE_NAME,                // pipe name 
            PIPE_ACCESS_DUPLEX,       // read/write access 
            PIPE_TYPE_MESSAGE |       // message type pipe 
            PIPE_READMODE_MESSAGE |   // message-read mode 
            PIPE_WAIT,                // blocking mode 
            PIPE_UNLIMITED_INSTANCES, // max. instances  
            BUF_SIZE,                 // output buffer size 
            BUF_SIZE,                 // input buffer size 
            0,                        // client time-out 
            NULL);                    // default security attribute 

        if (pipe == INVALID_HANDLE_VALUE) {
            wprintf(L"CreateNamedPipe failed (err %d).\n", GetLastError());
            return -1;
        }

        // Wait for the client to connect; if it succeeds, 
        // the function returns a nonzero value. If the function
        // returns zero, GetLastError returns ERROR_PIPE_CONNECTED. 
        BOOL connected = ConnectNamedPipe(pipe, NULL) ?
            TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

        if (connected) {
            printf("Client connected, creating a processing thread.\n");

            // Create a thread for this client. 
            DWORD  threadId = 0;
            HANDLE thread = CreateThread(
                NULL,            // no security attribute 
                0,               // default stack size 
                InstanceThread,  // thread proc
                pipe,            // thread parameter 
                0,               // not suspended 
                &threadId);      // returns thread ID 

            if (thread == NULL) {
                wprintf(L"CreateThread failed (err %d).\n", GetLastError());
                return -1;
            }

            CloseHandle(thread);
        } else {
            // The client could not connect, so close the pipe. 
            CloseHandle(pipe);
        }
    }

    return 0;
}

DWORD WINAPI InstanceThread(void* lpvParam)
// This routine is a thread processing function to read from and reply to a client
// via the open pipe connection passed from the main loop. Note this allows
// the main loop to continue executing, potentially creating more threads of
// of this procedure to run concurrently, depending on the number of incoming
// client connections.
{
    printf("InstanceThread created, receiving and processing messages.\n");

    std::vector<char> requestBuf(BUF_SIZE);
    std::vector<char> replyBuf(BUF_SIZE);

    // The thread's parameter is a handle to a pipe object instance. 
    HANDLE pipe = (HANDLE)lpvParam;

    // Loop until done reading
    for (;;) {
        // Read client requests from the pipe. This simplistic code only allows messages
        // up to BUFSIZE characters in length.
        DWORD bytesRead = 0;
        BOOL success = ReadFile(pipe,
            requestBuf.data(), // buffer to receive data 
            BUF_SIZE,     // size of buffer 
            &bytesRead, // number of bytes read 
            NULL);        // not overlapped I/O 

        if (!success || bytesRead == 0) {
            if (GetLastError() == ERROR_BROKEN_PIPE) {
                wprintf(L"InstanceThread: client disconnected.\n");
            } else {
                wprintf(L"InstanceThread ReadFile failed (err %d).\n", GetLastError());
            }
            break;
        }

        // Process the incoming message.
        wprintf(L"Client Request: %hs\n", requestBuf.data());

        // copy reply to output buffer
        HRESULT hr = StringCchCopyA(replyBuf.data(), BUF_SIZE, "Answer from server");
        if (FAILED(hr)) {
            replyBuf.clear();
            printf("ERROR: Output buffer too small.\n");
            return 1;
        }
        replyBuf.resize((strlen(replyBuf.data()) + 1)); // add zero termination


        // Write the reply to the pipe.
        DWORD bytesWritten = 0;
        success = WriteFile(pipe,
            replyBuf.data(), // buffer to write from 
            (DWORD)replyBuf.size(), // number of bytes to write 
            &bytesWritten,   // number of bytes written 
            NULL);        // not overlapped I/O 

        if (!success || (replyBuf.size() != bytesWritten)) {
            wprintf(L"InstanceThread WriteFile failed (err %d).\n", GetLastError());
            break;
        }
    }

    // flush & disconnect pipe before closing it
    FlushFileBuffers(pipe);
    DisconnectNamedPipe(pipe);
    CloseHandle(pipe);

    printf("InstanceThread exiting.\n");
    return 0;
}
