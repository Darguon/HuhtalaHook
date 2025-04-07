#include "Core/Cheats.h"
#include "Offsets/Offsets.h"
#include "Resources/Language.hpp"
#include "Core/Init.h"
#include "Config/ConfigSaver.h"
#include "Helpers/Logger.h"
#include <filesystem>
#include <KnownFolders.h>
#include <ShlObj.h>
#include "WebAuth.h"  // Include our web auth system instead of KeygenAuth directly
#include <wininet.h>  // For internet functions
#include <fstream>    // For file operations
#include <thread>     // For threading
#include <cstdio>     // For remove function

#pragma comment(lib, "wininet.lib")  // Link with wininet.lib

using namespace std;

namespace fs = filesystem;
string fileName;

// Function declarations
void Cheat();
bool CheckForUpdates();
bool DownloadAndInstallUpdate(const std::string& updateUrl, const std::string& version);
std::string GetCurrentVersion();
std::string GetLatestVersion();

// Configuration
const std::string VERSION = "1.1.0";  // Current version
const std::string UPDATE_SERVER = "http://51.83.133.131";  // Your Ubuntu VPS IP
const std::string UPDATE_URL = UPDATE_SERVER + "/updates/";  // Base URL for updates
const std::string VERSION_CHECK_URL = UPDATE_SERVER + "/version.txt";  // URL to check latest version

// Standard main function
int main(int argc, char* argv[])
{
    // Check if this is run after an update
    if (argc > 1 && std::string(argv[1]) == "--post-update") {
        // Delete the backup file (old version)
        std::string backupPath = argv[2];
        if (fs::exists(backupPath)) {
            try {
                fs::remove(backupPath);
                Log::Fine("Update completed successfully, removed backup: " + backupPath);
            }
            catch (const std::exception& e) {
                Log::Info("Note: Backup file could not be removed. Will be cleaned up later.");
            }
        }
    }

    Cheat();
    return 0;
}

// Windows-specific entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    return main(0, nullptr);
}

// Function to download content from a URL with error handling
std::string DownloadContent(const std::string& url) {
    HINTERNET hInternet = InternetOpenA("HuhtalaHook Updater", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) {
        DWORD error = GetLastError();
        Log::Warning("InternetOpen failed with error code: " + std::to_string(error), false);
        return "";
    }

    // Set timeout values (30 seconds)
    DWORD timeout = 30000;
    InternetSetOption(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOption(hInternet, INTERNET_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOption(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

    Log::Info("Connecting to: " + url);
    HINTERNET hConnect = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hConnect) {
        DWORD error = GetLastError();
        Log::Warning("InternetOpenUrl failed with error code: " + std::to_string(error), false);
        InternetCloseHandle(hInternet);
        return "";
    }

    // Check HTTP status code
    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    if (HttpQueryInfoA(hConnect, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &statusCodeSize, NULL)) {
        if (statusCode != 200) {
            Log::Warning("HTTP Error: " + std::to_string(statusCode), false);
            InternetCloseHandle(hConnect);
            InternetCloseHandle(hInternet);
            return "";
        }
    }

    char buffer[4096];
    DWORD bytesRead;
    std::string content;

    while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        content.append(buffer, bytesRead);
    }

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    if (content.empty()) {
        Log::Warning("Downloaded content is empty", false);
    }
    else {
        Log::Info("Content downloaded successfully (" + std::to_string(content.size()) + " bytes)");
    }

    return content;
}

// Function to get the current version
std::string GetCurrentVersion() {
    return VERSION;
}

// Function to get the latest version from the server
std::string GetLatestVersion() {
    std::string content = DownloadContent(VERSION_CHECK_URL);
    if (content.empty()) {
        return "";
    }

    // Simple text file with just the version number
    // Remove any whitespace or newlines
    content.erase(std::remove_if(content.begin(), content.end(),
        [](unsigned char c) { return std::isspace(c); }),
        content.end());

    return content;
}

// Function to compare version strings
bool IsNewVersionAvailable(const std::string& currentVersion, const std::string& latestVersion) {
    // Parse versions into components
    std::vector<int> current, latest;
    std::stringstream ss(currentVersion);
    std::string item;

    while (std::getline(ss, item, '.')) {
        current.push_back(std::stoi(item));
    }

    ss = std::stringstream(latestVersion);
    while (std::getline(ss, item, '.')) {
        latest.push_back(std::stoi(item));
    }

    // Pad with zeros if needed
    while (current.size() < latest.size()) {
        current.push_back(0);
    }
    while (latest.size() < current.size()) {
        latest.push_back(0);
    }

    // Compare version components
    for (size_t i = 0; i < current.size(); i++) {
        if (latest[i] > current[i]) {
            return true;
        }
        if (latest[i] < current[i]) {
            return false;
        }
    }

    return false;  // Versions are equal
}

// Function to check for updates
bool CheckForUpdates() {
    Log::Info("Checking for updates (" + VERSION_CHECK_URL + ")");

    std::string currentVersion = GetCurrentVersion();
    Log::Info("Current version: " + currentVersion);

    // Attempt to download version info
    std::string latestVersion;
    try {
        latestVersion = GetLatestVersion();
        if (latestVersion.empty()) {
            Log::PreviousLine();
            Log::Warning("Failed to get version info from server", false);
            return false;
        }
        Log::Info("Latest version from server: " + latestVersion);
    }
    catch (const std::exception& e) {
        Log::PreviousLine();
        Log::Warning("Exception during version check: " + std::string(e.what()), false);
        return false;
    }

    if (IsNewVersionAvailable(currentVersion, latestVersion)) {
        Log::PreviousLine();
        Log::Fine("Update available: v" + latestVersion);
        std::string updateFileUrl = UPDATE_URL + "HuhtalaHook-" + latestVersion + ".exe";
        Log::Info("Update URL: " + updateFileUrl);
        return DownloadAndInstallUpdate(updateFileUrl, latestVersion);
    }
    else {
        Log::PreviousLine();
        Log::Fine("You have the latest version: v" + currentVersion);
        return false;
    }
}

// Function to download and install the update
bool DownloadAndInstallUpdate(const std::string& updateUrl, const std::string& version) {
    Log::Info("Downloading update v" + version);

    // Get the current executable path
    char currentPath[MAX_PATH];
    GetModuleFileNameA(NULL, currentPath, MAX_PATH);
    std::string exePath = currentPath;

    // Create backup path and temporary path
    std::string backupPath = exePath + ".backup";
    std::string tempPath = exePath + ".update";

    // Download the update
    std::string updateData = DownloadContent(updateUrl);
    if (updateData.empty()) {
        Log::PreviousLine();
        Log::Error("Failed to download update");
        return false;
    }

    Log::PreviousLine();
    Log::Fine("Update downloaded successfully");
    Log::Info("Installing update");

    // Write the update to a temporary file
    std::ofstream tempFile(tempPath, std::ios::binary);
    if (!tempFile) {
        Log::PreviousLine();
        Log::Error("Failed to create temporary file");
        return false;
    }

    tempFile.write(updateData.c_str(), updateData.size());
    tempFile.close();

    // Create a batch file that will:
    // 1. Wait for the current process to exit
    // 2. Rename the current executable to .backup
    // 3. Rename the .update file to the executable name
    // 4. Start the new executable with --post-update flag

    std::string batchPath = MenuConfig::path + "\\update.bat";
    std::ofstream batchFile(batchPath);
    if (!batchFile) {
        Log::PreviousLine();
        Log::Error("Failed to create update script");
        return false;
    }

    batchFile << "@echo off\n";
    batchFile << "timeout /t 1 /nobreak >nul\n";  // Brief pause to ensure our process has time to exit
    batchFile << "move \"" << exePath << "\" \"" << backupPath << "\"\n";
    batchFile << "move \"" << tempPath << "\" \"" << exePath << "\"\n";
    batchFile << "start \"\" \"" << exePath << "\" --post-update \"" << backupPath << "\"\n";
    batchFile << "del \"%~f0\"\n";  // Delete the batch file
    batchFile.close();

    // Execute the batch file and exit
    STARTUPINFOA si = { sizeof(STARTUPINFOA) };
    PROCESS_INFORMATION pi;
    if (CreateProcessA(NULL, (LPSTR)batchPath.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        Log::PreviousLine();
        Log::Fine("Update ready. Restarting application...");
        Sleep(2000);
        exit(0);  // Exit to allow the update to proceed
        return true;
    }
    else {
        Log::PreviousLine();
        Log::Error("Failed to start update process");
        return false;
    }
}

// Function to authenticate with Keygen.sh (legacy system)
bool AuthenticateWithKeygen()
{
    // This is now handled by WebAuth
    // We'll just return true since WebAuth::StartWebAuth() already validated the license
    return true;
}

void Cheat()
{
    // Hide console window immediately - we'll use browser UI instead
    ShowWindow(GetConsoleWindow(), SW_HIDE);

    // Set title for debugging purposes only (won't be visible to user)
    SetConsoleTitle(L"HuhtalaHook");

    // Start web-based authentication and initialization
    if (!WebAuth::StartWebAuth())
    {
        // If web auth failed, show console for error messages
        ShowWindow(GetConsoleWindow(), SW_SHOWNORMAL);
        Log::Error("Authentication failed. Please restart the application.");
        Sleep(5000);
        exit(1);
        return;
    }

    // At this point, authentication and initialization are complete
    // The browser has already shown progress for all initialization steps
    
    // The console should remain hidden since we're using the browser UI
    ShowWindow(GetConsoleWindow(), SW_HIDE);

    try
    {
        Gui.AttachAnotherWindow("Counter-Strike 2", "SDL_app", Cheats::Run);
    }
    catch (OSImGui::OSException& e)
    {
        // Show console for error messages
        ShowWindow(GetConsoleWindow(), SW_SHOWNORMAL);
        Log::Error(e.what());
        Sleep(5000);
    }
}
