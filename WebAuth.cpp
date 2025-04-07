#include "WebAuth.h"
#include <WinSock2.h>  // Must come before Windows.h
#include <WS2tcpip.h>
#include <Windows.h>
#include <ShlObj.h>
#include <shellapi.h>
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <sstream>
#include <regex>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

#pragma comment(lib, "ws2_32.lib")

namespace {
    // Global variables for the web server
    std::atomic<bool> serverRunning(false);
    std::atomic<bool> authenticationComplete(false);
    std::atomic<int> authProgress(0);
    std::string authStatusMessage = "Waiting for authentication...";
    std::string validatedLicenseKey = "";
    std::mutex authMutex;
    std::condition_variable authCV;
    std::atomic<int> actualServerPort(8080);  // Track the actual port being used

    // Status tracking for initialization steps
    struct InitStatus {
        std::string message;
        std::string status; // "pending", "success", "error"
    };

    InitStatus versionStatus = {"Checking for updates...", "pending"};
    InitStatus offsetsStatus = {"Updating offsets...", "pending"};
    InitStatus driverStatus = {"Connecting to driver...", "pending"};
    InitStatus gameStatus = {"Waiting for game...", "pending"};

    // Configuration
    const int SERVER_PORT = 8080;  // Changed from 8085 to a more common port
    const std::string SERVER_HOST = "127.0.0.1";
    const std::string VERSION = "1.1.0";  // Current version
    
    // Fallback HTML content if the file can't be found
    const std::string FALLBACK_HTML = R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>HuhtalaHook Authentication</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
        }
        body {
            background-color: #0a0a0a;
            color: #f5f5f5;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            padding: 20px;
            background: linear-gradient(135deg, #0a0a0a 0%, #1a1a1a 100%);
        }
        .container {
            width: 100%;
            max-width: 450px;
            background-color: #1a1a1a;
            border-radius: 10px;
            box-shadow: 0 10px 30px rgba(0, 0, 0, 0.5);
            overflow: hidden;
        }
        .header {
            background-color: #121212;
            padding: 25px 20px;
            text-align: center;
            border-bottom: 1px solid #333;
        }
        .header h1 {
            font-size: 28px;
            margin-bottom: 5px;
            color: #fff;
        }
        .header p {
            color: #999;
            font-size: 16px;
        }
        .content {
            padding: 25px;
        }
        .form-group {
            margin-bottom: 20px;
        }
        label {
            display: block;
            margin-bottom: 8px;
            font-weight: 500;
            color: #ccc;
        }
        input {
            width: 100%;
            padding: 12px;
            background-color: #2a2a2a;
            border: 1px solid #444;
            border-radius: 5px;
            color: #fff;
            font-size: 16px;
            transition: border-color 0.2s;
        }
        input:focus {
            outline: none;
            border-color: #3498db;
            box-shadow: 0 0 0 2px rgba(52, 152, 219, 0.3);
        }
        button {
            width: 100%;
            padding: 12px;
            border: none;
            border-radius: 5px;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
            transition: background-color 0.2s;
        }
        .btn-primary {
            background-color: #3498db;
            color: white;
        }
        .btn-primary:hover {
            background-color: #2980b9;
        }
        .error-message {
            color: #e74c3c;
            margin-top: 10px;
            font-size: 14px;
        }
        .hidden {
            display: none;
        }
        .footer {
            background-color: #121212;
            padding: 12px 20px;
            text-align: center;
            border-top: 1px solid #333;
            font-size: 13px;
            color: #777;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>HuhtalaHook</h1>
            <p>Authentication Portal</p>
        </div>
        <div class="content">
            <div class="form-group">
                <label for="license-key">License Key</label>
                <input type="text" id="license-key" placeholder="Enter your license key">
            </div>
            <button id="verify-button" class="btn-primary">Verify License</button>
            <div id="auth-error" class="error-message hidden"></div>
        </div>
        <div class="footer">
            <span>Version: 1.1.0</span>
        </div>
    </div>
    <script>
        document.addEventListener('DOMContentLoaded', function() {
            const verifyButton = document.getElementById('verify-button');
            const licenseInput = document.getElementById('license-key');
            const authError = document.getElementById('auth-error');
            
            verifyButton.addEventListener('click', function() {
                const licenseKey = licenseInput.value.trim();
                if (!licenseKey) {
                    authError.textContent = "Please enter a valid license key";
                    authError.classList.remove('hidden');
                    return;
                }
                
                authError.classList.add('hidden');
                verifyButton.disabled = true;
                verifyButton.textContent = "Verifying...";
                
                fetch('/api/auth', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                    },
                    body: JSON.stringify({ licenseKey }),
                })
                .then(response => response.json())
                .then(data => {
                    if (data.success) {
                        verifyButton.textContent = "Authentication Successful";
                        verifyButton.style.backgroundColor = "#2ecc71";
                        setTimeout(() => {
                            window.close();
                        }, 2000);
                    } else {
                        authError.textContent = data.message || "Authentication failed";
                        authError.classList.remove('hidden');
                        verifyButton.disabled = false;
                        verifyButton.textContent = "Verify License";
                    }
                })
                .catch(error => {
                    authError.textContent = "Connection error. Please try again.";
                    authError.classList.remove('hidden');
                    verifyButton.disabled = false;
                    verifyButton.textContent = "Verify License";
                });
            });
        });
    </script>
</body>
</html>)";

    // Read the entire HTML file into a string
    std::string readHtmlFile(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Failed to open HTML file: " << path << std::endl;
            return "";
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    // Simple HTTP response builder
    std::string buildHttpResponse(int statusCode, const std::string& contentType, const std::string& body) {
        std::stringstream response;
        response << "HTTP/1.1 " << statusCode << " ";
        
        if (statusCode == 200) response << "OK";
        else if (statusCode == 404) response << "Not Found";
        else if (statusCode == 500) response << "Internal Server Error";
        else response << "Unknown";
        
        response << "\r\nContent-Type: " << contentType << "\r\n";
        response << "Content-Length: " << body.length() << "\r\n";
        response << "Connection: close\r\n";
        response << "Access-Control-Allow-Origin: *\r\n";
        response << "\r\n" << body;
        
        return response.str();
    }

    // Parse HTTP request to extract method, path and body
    bool parseHttpRequest(const std::string& request, std::string& method, std::string& path, std::string& body) {
        std::istringstream requestStream(request);
        std::string line;
        
        // Extract method and path from the first line
        if (std::getline(requestStream, line)) {
            std::istringstream lineStream(line);
            lineStream >> method >> path;
            std::cout << "Received " << method << " request for: " << path << std::endl;
        } else {
            std::cerr << "ERROR: Failed to parse HTTP request line" << std::endl;
            return false;
        }
        
        // Skip headers until we find an empty line
        int contentLength = 0;
        while (std::getline(requestStream, line) && line != "\r") {
            // Check for Content-Length header
            if (line.find("Content-Length:") != std::string::npos) {
                contentLength = std::stoi(line.substr(line.find(":") + 1));
                std::cout << "Request has content length: " << contentLength << " bytes" << std::endl;
            }
        }
        
        // Read body if Content-Length is specified
        if (contentLength > 0) {
            char* bodyBuffer = new char[contentLength + 1];
            requestStream.read(bodyBuffer, contentLength);
            bodyBuffer[contentLength] = '\0';
            body = std::string(bodyBuffer);
            delete[] bodyBuffer;
            
            // Don't log the entire body as it might contain sensitive information
            std::cout << "Request body received (" << body.length() << " bytes)" << std::endl;
        }
        
        return true;
    }
    
    // Extract license key from JSON body
    std::string extractLicenseKey(const std::string& jsonBody) {
        std::regex keyPattern("\"licenseKey\"\\s*:\\s*\"([^\"]+)\"");
        std::smatch matches;
        
        if (std::regex_search(jsonBody, matches, keyPattern) && matches.size() > 1) {
            return matches[1].str();
        }
        
        return "";
    }
    
    // Validate license key (replace with your actual validation logic)
    bool validateLicenseKey(const std::string& key) {
        // For demonstration, we'll accept any key that's at least 8 characters
        // In a real application, you would validate against a database or API
        return key.length() >= 8;
    }
    
    // Save license key to file
    bool saveLicenseKey(const std::string& key, const std::string& configPath) {
        std::string licensePath = configPath + "\\license.key";
        std::ofstream licenseFile(licensePath);
        
        if (licenseFile.is_open()) {
            licenseFile << key;
            licenseFile.close();
            std::cout << "License key saved to: " << licensePath << std::endl;
            return true;
        }
        
        std::cerr << "Failed to save license key to: " << licensePath << std::endl;
        return false;
    }
    
    // Handle HTTP requests
    void handleClient(SOCKET clientSocket, const std::string& htmlContent, const std::string& configPath) {
        char buffer[8192] = {0};
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            std::string request(buffer);
            
            std::cout << "\n--- New Client Connection ---" << std::endl;
            std::cout << "Received " << bytesReceived << " bytes from client" << std::endl;
            
            std::string method, path, body;
            if (parseHttpRequest(request, method, path, body)) {
                std::string response;
                
                // Handle different endpoints
                if (path == "/" || path == "/index.html") {
                    // Serve the main HTML page
                    std::cout << "Serving main HTML page" << std::endl;
                    response = buildHttpResponse(200, "text/html", htmlContent);
                }
                else if (path == "/api/status") {
                    // Return current authentication status
                    std::stringstream jsonResponse;
                    jsonResponse << "{\"progress\":" << authProgress.load();
                    jsonResponse << ",\"message\":\"" << authStatusMessage << "\"";
                    jsonResponse << ",\"completed\":" << (authProgress.load() >= 100 ? "true" : "false");
                    jsonResponse << ",\"version\":\"" << VERSION << "\"";
                    
                    // Add initialization status
                    jsonResponse << ",\"versionStatus\":{\"message\":\"" << versionStatus.message 
                                << "\",\"status\":\"" << versionStatus.status << "\"}";
                    
                    jsonResponse << ",\"offsetsStatus\":{\"message\":\"" << offsetsStatus.message 
                                << "\",\"status\":\"" << offsetsStatus.status << "\"}";
                    
                    jsonResponse << ",\"driverStatus\":{\"message\":\"" << driverStatus.message 
                                << "\",\"status\":\"" << driverStatus.status << "\"}";
                    
                    jsonResponse << ",\"gameStatus\":{\"message\":\"" << gameStatus.message 
                                << "\",\"status\":\"" << gameStatus.status << "\"}";
                    
                    jsonResponse << "}";
                    
                    std::cout << "Status request - Progress: " << authProgress.load() 
                              << "%, Message: " << authStatusMessage << std::endl;
                    
                    response = buildHttpResponse(200, "application/json", jsonResponse.str());
                }
                else if (path == "/api/auth" && method == "POST") {
                    // Handle authentication request
                    std::string licenseKey = extractLicenseKey(body);
                    std::cout << "Authentication request received, key length: " << licenseKey.length() << std::endl;
                    
                    bool isValid = validateLicenseKey(licenseKey);
                    std::cout << "License key validation: " << (isValid ? "VALID" : "INVALID") << std::endl;
                    
                    if (isValid) {
                        // Update authentication status
                        authStatusMessage = "License key validated. Initializing...";
                        authProgress.store(10);
                        std::cout << "Authentication successful, beginning initialization process" << std::endl;
                        
                        // Save the validated key
                        validatedLicenseKey = licenseKey;
                        if (saveLicenseKey(licenseKey, configPath)) {
                            std::cout << "License key saved successfully" << std::endl;
                            
                            // Simulate initialization process
                            std::thread([&]() {
                                // Simulate progress
                                std::cout << "Starting initialization process in background thread" << std::endl;
                                
                                // Authentication complete - 10%
                                authProgress.store(10);
                                authStatusMessage = "License key validated. Initializing...";
                                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                                
                                // Check for updates - 20%
                                authProgress.store(20);
                                authStatusMessage = "Checking for updates...";
                                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                                versionStatus.message = "Your cheat version is up to date and supported";
                                versionStatus.status = "success";
                                
                                // Update offsets - 40%
                                authProgress.store(40);
                                authStatusMessage = "Updating offsets...";
                                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                                offsetsStatus.message = "Offsets updated successfully";
                                offsetsStatus.status = "success";
                                
                                // Connect to driver - 60%
                                authProgress.store(60);
                                authStatusMessage = "Connecting to driver...";
                                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                                driverStatus.message = "Successfully connected to kernel mode driver";
                                driverStatus.status = "success";
                                
                                // Wait for game - 80%
                                authProgress.store(80);
                                authStatusMessage = "Waiting for game...";
                                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                                gameStatus.message = "Connected to CS2";
                                gameStatus.status = "success";
                                
                                // Ready to launch - 100%
                                authProgress.store(100);
                                authStatusMessage = "Ready to launch!";
                                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                                
                                // Mark authentication as complete
                                authenticationComplete.store(true);
                                std::cout << "Progress 100%: Authentication and initialization complete" << std::endl;
                                
                                // Notify waiting threads
                                std::unique_lock<std::mutex> lock(authMutex);
                                authCV.notify_all();
                            }).detach();
                            
                            response = buildHttpResponse(200, "application/json", "{\"success\":true,\"message\":\"Authentication successful\"}");
                        } else {
                            std::cerr << "ERROR: Failed to save license key" << std::endl;
                            response = buildHttpResponse(500, "application/json", "{\"success\":false,\"message\":\"Failed to save license key\"}");
                        }
                    } else {
                        std::cout << "Invalid license key rejected" << std::endl;
                        response = buildHttpResponse(200, "application/json", "{\"success\":false,\"message\":\"Invalid license key\"}");
                    }
                }
                else if (path == "/api/launch" && method == "POST") {
                    // Handle launch request
                    std::cout << "Launch request received" << std::endl;
                    if (authenticationComplete.load()) {
                        // Signal to stop the server
                        std::cout << "Authentication is complete, signaling server to stop" << std::endl;
                        serverRunning.store(false);
                        response = buildHttpResponse(200, "application/json", "{\"success\":true,\"message\":\"Launching application\"}");
                    } else {
                        std::cout << "Launch request rejected - authentication not complete" << std::endl;
                        response = buildHttpResponse(200, "application/json", "{\"success\":false,\"message\":\"Authentication not complete\"}");
                    }
                }
                else {
                    // 404 for unknown paths
                    std::cout << "Unknown path requested: " << path << std::endl;
                    response = buildHttpResponse(404, "text/plain", "Not Found");
                }
                
                // Send the response
                std::cout << "Sending response (" << response.length() << " bytes)" << std::endl;
                int bytesSent = send(clientSocket, response.c_str(), static_cast<int>(response.length()), 0);
                if (bytesSent == SOCKET_ERROR) {
                    std::cerr << "ERROR: Failed to send response: " << WSAGetLastError() << std::endl;
                } else {
                    std::cout << "Response sent successfully: " << bytesSent << " bytes" << std::endl;
                }
            } else {
                std::cerr << "ERROR: Failed to parse HTTP request" << std::endl;
            }
        } else if (bytesReceived == 0) {
            std::cout << "Client closed connection" << std::endl;
        } else {
            std::cerr << "ERROR: recv failed: " << WSAGetLastError() << std::endl;
        }
        
        // Close the client socket
        std::cout << "Closing client connection" << std::endl;
        closesocket(clientSocket);
    }
    
    // Start the web server
    void startWebServer(const std::string& htmlContent, const std::string& configPath) {
        std::cout << "\n=== Starting Web Server ===\n" << std::endl;
        
        // Initialize Winsock
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "ERROR: Failed to initialize Winsock" << std::endl;
            return;
        }
        std::cout << "Winsock initialized successfully" << std::endl;
        
        // Create socket
        SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (serverSocket == INVALID_SOCKET) {
            std::cerr << "Failed to create socket: " << WSAGetLastError() << std::endl;
            WSACleanup();
            return;
        }
        
        // Set socket option to allow reuse of address/port
        int optval = 1;
        if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof(optval)) == SOCKET_ERROR) {
            std::cerr << "Failed to set SO_REUSEADDR: " << WSAGetLastError() << std::endl;
            // Continue anyway, this is not critical
        }
        
        // Setup server address
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(SERVER_PORT);
        serverAddr.sin_addr.s_addr = INADDR_ANY;  // Accept connections on any interface
        
        // Bind socket
        if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            int error = WSAGetLastError();
            std::cerr << "Failed to bind socket: " << error << std::endl;
            
            // If port is in use, try another port
            if (error == WSAEADDRINUSE) {
                std::cerr << "Port " << SERVER_PORT << " is already in use. Trying alternate port." << std::endl;
                
                // Try an alternate port
                int alternatePort = SERVER_PORT + 1;
                serverAddr.sin_port = htons(alternatePort);
                if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
                    std::cerr << "Failed to bind to alternate port: " << WSAGetLastError() << std::endl;
                    closesocket(serverSocket);
                    WSACleanup();
                    return;
                }
                
                // Update the actual port being used
                actualServerPort.store(alternatePort);
                std::cout << "Successfully bound to alternate port: " << alternatePort << std::endl;
            } else {
                closesocket(serverSocket);
                WSACleanup();
                return;
            }
        }
        
        std::cout << "Bound to address: " << SERVER_HOST << ":" << actualServerPort.load() << std::endl;
        
        // Listen for connections
        if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "Failed to listen: " << WSAGetLastError() << std::endl;
            closesocket(serverSocket);
            WSACleanup();
            return;
        }
        
        std::cout << "Web server started at http://" << SERVER_HOST << ":" << actualServerPort.load() << std::endl;
        serverRunning.store(true);
        
        // Set socket to non-blocking mode
        u_long mode = 1;
        ioctlsocket(serverSocket, FIONBIO, &mode);
        
        // Accept connections until serverRunning is false
        while (serverRunning.load()) {
            // Accept a client connection
            sockaddr_in clientAddr;
            int clientAddrSize = sizeof(clientAddr);
            SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);
            
            if (clientSocket != INVALID_SOCKET) {
                // Handle client in a separate thread
                std::thread([=]() {
                    handleClient(clientSocket, htmlContent, configPath);
                }).detach();
            }
            else {
                // Check if it's a non-blocking error or a real error
                int error = WSAGetLastError();
                if (error != WSAEWOULDBLOCK) {
                    std::cerr << "Accept failed: " << error << std::endl;
                    break;
                }
            }
            
            // Sleep to prevent high CPU usage
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        // Cleanup
        closesocket(serverSocket);
        WSACleanup();
        std::cout << "Web server stopped" << std::endl;
    }
    
    // Open URL in default browser
    bool openBrowser(const std::string& url) {
        std::cout << "Attempting to open browser at URL: " << url << std::endl;
        
        // Use ShellExecute to open the URL in the default browser
        HINSTANCE result = ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
        
        if ((intptr_t)result <= 32) {
            // Error occurred
            std::cerr << "ERROR: ShellExecute failed with error code: " << (intptr_t)result << std::endl;
            return false;
        }
        
        std::cout << "Browser opened successfully" << std::endl;
        return true;
    }
}

// Implementation of WebAuth::StartWebAuth
bool WebAuth::StartWebAuth() {
    std::cout << "Starting browser-based authentication..." << std::endl;
    
    // For testing: Set to true to force browser authentication even if a valid license exists
    const bool FORCE_BROWSER_AUTH = true;
    
    try {
        // Get documents path for config and license storage
        char documentsPath[MAX_PATH];
        if (SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, 0, documentsPath) != S_OK) {
            std::cerr << "ERROR: Failed to get Documents path" << std::endl;
            return false;
        }
        
        std::string configPath = std::string(documentsPath) + "\\HuhtalaHook";
        std::cout << "Config path: " << configPath << std::endl;
        
        // Create config directory if it doesn't exist
        if (!std::filesystem::exists(configPath)) {
            std::cout << "Creating config directory: " << configPath << std::endl;
            if (!std::filesystem::create_directory(configPath)) {
                std::cerr << "ERROR: Failed to create config directory" << std::endl;
                return false;
            }
        }
        
        // Check if license key already exists
        std::string licensePath = configPath + "\\license.key";
        bool skipAuth = false;
        
        if (!FORCE_BROWSER_AUTH && std::filesystem::exists(licensePath)) {
            std::cout << "Found existing license file: " << licensePath << std::endl;
            std::ifstream licenseFile(licensePath);
            if (licenseFile.is_open()) {
                std::string licenseKey;
                std::getline(licenseFile, licenseKey);
                licenseFile.close();
                
                if (!licenseKey.empty() && validateLicenseKey(licenseKey)) {
                    std::cout << "Valid license key found" << std::endl;
                    if (!FORCE_BROWSER_AUTH) {
                        std::cout << "Skipping authentication" << std::endl;
                        skipAuth = true;
                        return true;
                    } else {
                        std::cout << "TESTING MODE: Forcing browser authentication despite valid license" << std::endl;
                    }
                } else {
                    std::cout << "Invalid or empty license key found, continuing with authentication" << std::endl;
                }
            }
        } else {
            if (FORCE_BROWSER_AUTH) {
                std::cout << "TESTING MODE: Forcing browser authentication" << std::endl;
            } else {
                std::cout << "No existing license file found" << std::endl;
            }
        }
        
        // Read the HTML file
        std::string currentPath = std::filesystem::current_path().string();
        std::string htmlPath = currentPath + "\\index.html";
        std::cout << "Looking for HTML file at: " << htmlPath << std::endl;
        
        std::string htmlContent;
        
        if (!std::filesystem::exists(htmlPath)) {
            std::cerr << "WARNING: HTML file not found at: " << htmlPath << std::endl;
            
            // Try to find the HTML file in other locations
            std::cout << "Searching for index.html in current directory..." << std::endl;
            bool foundHtml = false;
            for (const auto& entry : std::filesystem::directory_iterator(currentPath)) {
                std::cout << "Found file: " << entry.path().string() << std::endl;
                if (entry.path().filename() == "index.html") {
                    htmlPath = entry.path().string();
                    foundHtml = true;
                    std::cout << "Found index.html at: " << htmlPath << std::endl;
                    break;
                }
            }
            
            if (!foundHtml) {
                std::cout << "Using fallback HTML content" << std::endl;
                htmlContent = FALLBACK_HTML;
            } else {
                htmlContent = readHtmlFile(htmlPath);
            }
        } else {
            htmlContent = readHtmlFile(htmlPath);
        }
        
        if (htmlContent.empty()) {
            std::cerr << "ERROR: Failed to read HTML file or file is empty, using fallback" << std::endl;
            htmlContent = FALLBACK_HTML;
        }
        
        std::cout << "HTML content ready, size: " << htmlContent.size() << " bytes" << std::endl;
        
        // Start the web server in a separate thread
        std::cout << "Starting web server thread..." << std::endl;
        std::thread serverThread([&]() {
            startWebServer(htmlContent, configPath);
        });
        
        // Give the server a moment to start
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Open the browser with the actual port being used
        std::string url = "http://" + SERVER_HOST + ":" + std::to_string(actualServerPort.load());
        std::cout << "Opening browser at: " << url << std::endl;
        
        if (!openBrowser(url)) {
            std::cerr << "ERROR: Failed to open browser" << std::endl;
            serverRunning.store(false);
            serverThread.join();
            return false;
        }
        
        std::cout << "Waiting for authentication to complete..." << std::endl;
        
        // Wait for authentication to complete
        {
            std::unique_lock<std::mutex> lock(authMutex);
            authCV.wait(lock, []{ return authenticationComplete.load(); });
        }
        
        std::cout << "Authentication completed, stopping server..." << std::endl;
        
        // Wait for server thread to finish
        if (serverThread.joinable()) {
            serverThread.join();
        }
        
        std::cout << "Server stopped, authentication process complete" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in WebAuth: " << e.what() << std::endl;
        return false;
    }
    catch (...) {
        std::cerr << "Unknown error in WebAuth" << std::endl;
        return false;
    }
}