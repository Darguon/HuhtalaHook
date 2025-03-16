#pragma once
#include <string>
#include <Windows.h>
#include <winhttp.h>
#include <sstream>

#pragma comment(lib, "winhttp.lib")

class SimpleAuth {
private:
    // Response data
    struct Response {
        bool success = false;
        std::string message = "";
    };

    // Configuration
    std::string name;
    std::string ownerid;
    std::string secret;
    std::string version;
    std::string url;
    std::string domain;
    std::string path;
    std::string sessionid; // Store the session ID
    bool initialized = false;
    Response response;

    // Internal helper functions
    std::wstring StringToWideString(const std::string& str) {
        if (str.empty()) {
            return std::wstring();
        }

        // Calculate the required buffer size
        int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
        std::wstring wstr(size - 1, 0); // -1 because size includes the null terminator

        // Convert the string
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size);

        return wstr;
    }

    std::string GetHWID() {
        char volumeName[MAX_PATH + 1] = { 0 };
        char fileSystemName[MAX_PATH + 1] = { 0 };
        DWORD serialNumber = 0;
        DWORD maxComponentLen = 0;
        DWORD fileSystemFlags = 0;

        if (GetVolumeInformationA("C:\\", volumeName, ARRAYSIZE(volumeName),
            &serialNumber, &maxComponentLen, &fileSystemFlags,
            fileSystemName, ARRAYSIZE(fileSystemName))) {
            return std::to_string(serialNumber);
        }

        return "UNKNOWN";
    }

    std::string Request(const std::string& data) {
        std::string responseStr;

        // Initialize WinHTTP
        HINTERNET hSession = WinHttpOpen(L"HuhtalaHook/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS, 0);

        if (!hSession) {
            return "{\"success\":false,\"message\":\"Failed to initialize WinHTTP\"}";
        }

        // Connect to server
        std::wstring wideDomain = StringToWideString(domain);
        HINTERNET hConnect = WinHttpConnect(hSession, wideDomain.c_str(),
            INTERNET_DEFAULT_HTTPS_PORT, 0);

        if (!hConnect) {
            WinHttpCloseHandle(hSession);
            return "{\"success\":false,\"message\":\"Failed to connect to server\"}";
        }

        // Create a request
        std::wstring widePath = StringToWideString(path);
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", widePath.c_str(),
            NULL, WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE);

        if (!hRequest) {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return "{\"success\":false,\"message\":\"Failed to create request\"}";
        }

        // Set headers
        const wchar_t* headers = L"Content-Type: application/x-www-form-urlencoded\r\n";
        DWORD headersLen = (DWORD)wcslen(headers);

        if (!WinHttpAddRequestHeaders(hRequest, headers, headersLen, WINHTTP_ADDREQ_FLAG_ADD)) {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return "{\"success\":false,\"message\":\"Failed to set headers\"}";
        }

        // Send the request
        DWORD dataLen = (DWORD)data.length();
        BOOL result = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
            (LPVOID)data.c_str(), dataLen, dataLen, 0);

        if (!result) {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return "{\"success\":false,\"message\":\"Failed to send request\"}";
        }

        // Receive the response
        result = WinHttpReceiveResponse(hRequest, NULL);

        if (result) {
            DWORD dwSize = 0;
            DWORD dwDownloaded = 0;
            LPSTR pszOutBuffer;

            do {
                // Check for available data
                dwSize = 0;
                if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
                    break;
                }

                // No more data
                if (dwSize == 0) {
                    break;
                }

                // Allocate space for the buffer
                pszOutBuffer = new char[dwSize + 1];
                if (!pszOutBuffer) {
                    break;
                }

                // Read the data
                ZeroMemory(pszOutBuffer, dwSize + 1);

                if (!WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded)) {
                    delete[] pszOutBuffer;
                    break;
                }

                // Append to response
                responseStr.append(pszOutBuffer, dwDownloaded);
                delete[] pszOutBuffer;
            } while (dwSize > 0);
        }

        // Clean up
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        if (responseStr.empty()) {
            return "{\"success\":false,\"message\":\"Empty response from server\"}";
        }

        return responseStr;
    }

    void ParseResponse(const std::string& responseStr) {
        // Check for success flag
        if (responseStr.find("\"success\":true") != std::string::npos) {
            response.success = true;

            // Extract sessionid if it exists
            size_t sessionStart = responseStr.find("\"sessionid\":\"");
            if (sessionStart != std::string::npos) {
                sessionStart += 13; // Length of "\"sessionid\":\""
                size_t sessionEnd = responseStr.find("\"", sessionStart);
                if (sessionEnd != std::string::npos) {
                    sessionid = responseStr.substr(sessionStart, sessionEnd - sessionStart);
                }
            }

            // Extract message if available
            size_t msgStart = responseStr.find("\"message\":\"");
            if (msgStart != std::string::npos) {
                msgStart += 11; // Length of "\"message\":\""
                size_t msgEnd = responseStr.find("\"", msgStart);
                if (msgEnd != std::string::npos) {
                    response.message = responseStr.substr(msgStart, msgEnd - msgStart);
                }
                else {
                    response.message = "License valid";
                }
            }
            else {
                response.message = "License valid";
            }
        }
        else {
            response.success = false;
            // Extract error message if available
            size_t msgStart = responseStr.find("\"message\":\"");
            if (msgStart != std::string::npos) {
                msgStart += 11; // Length of "\"message\":\""
                size_t msgEnd = responseStr.find("\"", msgStart);
                if (msgEnd != std::string::npos) {
                    response.message = responseStr.substr(msgStart, msgEnd - msgStart);
                }
                else {
                    response.message = "Invalid license";
                }
            }
            else {
                response.message = "Invalid license";
            }
        }
    }

public:
    // Constructor and destructor
    SimpleAuth(const std::string& name, const std::string& ownerid, const std::string& secret,
        const std::string& version, const std::string& url)
        : name(name), ownerid(ownerid), secret(secret), version(version), url(url) {
        // Parse the URL to get domain and path
        std::string tempUrl = url;
        if (tempUrl.substr(0, 8) == "https://") {
            tempUrl = tempUrl.substr(8);
        }
        else if (tempUrl.substr(0, 7) == "http://") {
            tempUrl = tempUrl.substr(7);
        }

        size_t slashPos = tempUrl.find('/');
        if (slashPos != std::string::npos) {
            domain = tempUrl.substr(0, slashPos);
            path = tempUrl.substr(slashPos);
        }
        else {
            domain = tempUrl;
            path = "/";
        }
    }

    ~SimpleAuth() {
        // No cleanup needed for WinHTTP as we close handles after each request
    }

    // Core functionality
    bool Init() {
        // Build request data
        std::string data = "type=init";
        data += "&name=" + name;
        data += "&ownerid=" + ownerid;
        data += "&ver=" + version;

        // Send request
        std::string responseStr = Request(data);
        ParseResponse(responseStr);

        if (response.success) {
            initialized = true;
        }

        return response.success;
    }

    bool VerifyLicense(const std::string& key) {
        if (!initialized) {
            response.success = false;
            response.message = "Auth not initialized";
            return false;
        }

        // Build request data
        std::string data = "type=license";
        data += "&key=" + key;
        data += "&hwid=" + GetHWID();
        data += "&sessionid=" + sessionid; // Include session ID
        data += "&name=" + name;
        data += "&ownerid=" + ownerid;

        // Send request
        std::string responseStr = Request(data);
        ParseResponse(responseStr);

        return response.success;
    }

    // Accessors for response data
    bool IsSuccess() const {
        return response.success;
    }

    std::string GetMessage() const {
        return response.message;
    }
};