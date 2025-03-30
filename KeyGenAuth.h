#pragma once
#include <string>
#include <Windows.h>
#include <winhttp.h>
#include <sstream>
#include <vector>
#include <memory>
#include <stdexcept>
#include <iostream>

#pragma comment(lib, "winhttp.lib")

class KeygenAuth {
private:
    // Configuration
    std::string account_id;
    std::string product_id;
    std::string api_key;
    std::string hwid;

    // Response data
    struct Response {
        bool success = false;
        std::string message = "";
        std::string license_key = "";
        std::string license_id = "";
    };

    Response response;
    bool initialized = false;

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

    std::string Request(const std::string& endpoint, const std::string& method, const std::string& data = "") {
        std::string responseStr;

        // Construct the full URL properly
        std::string fullUrl = "https://api.keygen.sh" + endpoint;

        // For debugging
        std::cout << "Sending " << method << " request to: " << fullUrl << std::endl;
        if (!data.empty()) {
            std::cout << "Request data: " << data << std::endl;
        }

        // Initialize WinHTTP
        HINTERNET hSession = WinHttpOpen(L"HuhtalaHook/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS, 0);

        if (!hSession) {
            std::cout << "Failed to initialize WinHTTP: " << GetLastError() << std::endl;
            return "{\"meta\":{\"success\":false,\"message\":\"Failed to initialize WinHTTP\"}}";
        }

        // Connect to server
        std::wstring wideDomain = StringToWideString("api.keygen.sh");
        HINTERNET hConnect = WinHttpConnect(hSession, wideDomain.c_str(),
            INTERNET_DEFAULT_HTTPS_PORT, 0);

        if (!hConnect) {
            std::cout << "Failed to connect to server: " << GetLastError() << std::endl;
            WinHttpCloseHandle(hSession);
            return "{\"meta\":{\"success\":false,\"message\":\"Failed to connect to server\"}}";
        }

        // Create a request with the correct path
        std::wstring widePath = StringToWideString(endpoint);
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, StringToWideString(method).c_str(), widePath.c_str(),
            NULL, WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE);

        if (!hRequest) {
            std::cout << "Failed to create request: " << GetLastError() << std::endl;
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return "{\"meta\":{\"success\":false,\"message\":\"Failed to create request\"}}";
        }

        // Set headers
        std::string headers = "Content-Type: application/vnd.api+json\r\nAccept: application/vnd.api+json\r\n";
        // Only add Authorization header if API key is provided
        if (!api_key.empty()) {
            headers += "Authorization: Bearer " + api_key + "\r\n";
        }
        std::wstring wideHeaders = StringToWideString(headers);
        DWORD headersLen = (DWORD)wideHeaders.length();

        if (!WinHttpAddRequestHeaders(hRequest, wideHeaders.c_str(), headersLen, WINHTTP_ADDREQ_FLAG_ADD)) {
            std::cout << "Failed to set headers: " << GetLastError() << std::endl;
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return "{\"meta\":{\"success\":false,\"message\":\"Failed to set headers\"}}";
        }

        // Send the request
        BOOL result;
        if (method == "GET") {
            result = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
        }
        else {
            DWORD dataLen = (DWORD)data.length();
            result = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                (LPVOID)data.c_str(), dataLen, dataLen, 0);
        }

        if (!result) {
            std::cout << "Failed to send request: " << GetLastError() << std::endl;
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return "{\"meta\":{\"success\":false,\"message\":\"Failed to send request\"}}";
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
            return "{\"meta\":{\"success\":false,\"message\":\"Empty response from server\"}}";
        }

        std::cout << "License validation response: " << responseStr << std::endl;
        return responseStr;
    }

    bool ParseValidationResponse(const std::string& responseStr) {
        // A very simple JSON response parser specifically for Keygen responses
        // Note: In a production app, you should use a proper JSON library

        // First check if we have a valid response
        if (responseStr.empty()) {
            response.success = false;
            response.message = "Empty response from server";
            return false;
        }

        // Check for success in validation response
        if (responseStr.find("\"valid\":true") != std::string::npos) {
            response.success = true;

            // Extract license key
            size_t keyStart = responseStr.find("\"key\":\"");
            if (keyStart != std::string::npos) {
                keyStart += 7; // Length of "\"key\":\""
                size_t keyEnd = responseStr.find("\"", keyStart);
                if (keyEnd != std::string::npos) {
                    response.license_key = responseStr.substr(keyStart, keyEnd - keyStart);
                }
            }

            // Extract license ID
            size_t idStart = responseStr.find("\"id\":\"");
            if (idStart != std::string::npos) {
                idStart += 6; // Length of "\"id\":\""
                size_t idEnd = responseStr.find("\"", idStart);
                if (idEnd != std::string::npos) {
                    response.license_id = responseStr.substr(idStart, idEnd - idStart);
                }
            }

            response.message = "License is valid";
            return true;
        }
        else {
            response.success = false;

            // Check for various error scenarios
            if (responseStr.find("\"errors\"") != std::string::npos) {
                // API error response
                size_t errorStart = responseStr.find("\"detail\":\"");
                if (errorStart != std::string::npos) {
                    errorStart += 10; // Length of "\"detail\":\""
                    size_t errorEnd = responseStr.find("\"", errorStart);
                    if (errorEnd != std::string::npos) {
                        response.message = responseStr.substr(errorStart, errorEnd - errorStart);
                    }
                }
                else if (responseStr.find("\"code\":\"") != std::string::npos) {
                    // Try to extract error code
                    size_t codeStart = responseStr.find("\"code\":\"");
                    codeStart += 8; // Length of "\"code\":\""
                    size_t codeEnd = responseStr.find("\"", codeStart);
                    if (codeEnd != std::string::npos) {
                        std::string errorCode = responseStr.substr(codeStart, codeEnd - codeStart);
                        response.message = "Error code: " + errorCode;
                    }
                }
                else {
                    response.message = "API error response";
                }
            }
            else if (responseStr.find("\"valid\":false") != std::string::npos) {
                response.message = "License key is invalid";
            }
            else {
                response.message = "Invalid license or server error";
            }

            return false;
        }
    }

public:
    // Constructor
    KeygenAuth(const std::string& account_id, const std::string& product_id, const std::string& api_key = "")
        : account_id(account_id), product_id(product_id), api_key(api_key),
        hwid(GetHWID()) {
    }

    // Initialize the authentication system
    bool Init() {
        // Skip API connectivity check - it's not needed for basic license validation
        initialized = true;
        response.success = true;
        response.message = "Keygen.sh authentication initialized";
        return true;
    }

    // Validate a license key
    bool ValidateLicense(const std::string& key) {
        // Use the public license validation endpoint which doesn't require authentication
        // Note: Fixed the endpoint to avoid duplication of /v1
        std::string endpoint = "/v1/accounts/" + account_id + "/licenses/actions/validate-key";

        // Construct the JSON request - simpler version without fingerprint
        std::string data = "{\n";
        data += "  \"meta\": {\n";
        data += "    \"key\": \"" + key + "\"\n";
        data += "  }\n";
        data += "}";

        std::string responseStr = Request(endpoint, "POST", data);
        return ParseValidationResponse(responseStr);
    }

    // Activate a license for this machine (optional)
    bool ActivateLicense(const std::string& key) {
        // First validate the license
        if (!ValidateLicense(key)) {
            return false;
        }

        // Then create an activation
        std::string endpoint = "/v1/accounts/" + account_id + "/licenses/" + response.license_id + "/machines";

        // Construct the JSON request
        std::string data = "{\n";
        data += "  \"data\": {\n";
        data += "    \"type\": \"machines\",\n";
        data += "    \"attributes\": {\n";
        data += "      \"fingerprint\": \"" + hwid + "\",\n";
        data += "      \"platform\": \"windows\"\n";
        data += "    },\n";
        data += "    \"relationships\": {\n";
        data += "      \"license\": {\n";
        data += "        \"data\": {\n";
        data += "          \"type\": \"licenses\",\n";
        data += "          \"id\": \"" + response.license_id + "\"\n";
        data += "        }\n";
        data += "      }\n";
        data += "    }\n";
        data += "  }\n";
        data += "}";

        std::string responseStr = Request(endpoint, "POST", data);

        if (responseStr.find("\"id\":\"") != std::string::npos) {
            response.success = true;
            response.message = "License activated successfully";
            return true;
        }
        else {
            response.success = false;

            // Try to extract error message
            size_t errorStart = responseStr.find("\"detail\":\"");
            if (errorStart != std::string::npos) {
                errorStart += 10; // Length of "\"detail\":\""
                size_t errorEnd = responseStr.find("\"", errorStart);
                if (errorEnd != std::string::npos) {
                    response.message = responseStr.substr(errorStart, errorEnd - errorStart);
                }
            }
            else {
                response.message = "Failed to activate license";
            }

            return false;
        }
    }

    // Getters for response data
    bool IsSuccess() const {
        return response.success;
    }

    std::string GetMessage() const {
        return response.message;
    }

    std::string GetLicenseKey() const {
        return response.license_key;
    }

    std::string GetLicenseId() const {
        return response.license_id;
    }
};