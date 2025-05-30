#include "Core/Cheats.h"
#include "Offsets/Offsets.h"
#include "Resources/Language.hpp"
#include "Core/Init.h"
#include "Config/ConfigSaver.h"
#include "Helpers/Logger.h"
#include <filesystem>
#include <KnownFolders.h>
#include <ShlObj.h>
#include "KeygenAuth.h"  // Include our Keygen auth header
#include <wininet.h>     // For internet functions
#include <fstream>       // For file operations
#include <thread>        // For threading
#include <cstdio>        // For remove function
#include <mmsystem.h>    // For playing MP3

#pragma comment(lib, "wininet.lib")  // Link with wininet.lib
#pragma comment(lib, "winmm.lib")    // Link with Windows multimedia library

using namespace std;

namespace fs = filesystem;
string fileName;

// Function declarations
void Cheat();
bool AuthenticateWithKeygen();
bool CheckForUpdates();
bool DownloadAndInstallUpdate(const std::string& updateUrl, const std::string& version);
std::string GetCurrentVersion();
std::string GetLatestVersion();
bool PlayBackgroundMusic(const std::string& musicFileName);
void StopBackgroundMusic();

// Global variables for music playback
std::thread g_musicThread;
bool g_musicPlaying = false;
std::string g_musicPath;

// Configuration
const std::string VERSION = "1.8.0";  // Current version
const std::string UPDATE_SERVER = "http://51.83.133.131";  // Your Ubuntu VPS IP
const std::string UPDATE_URL = UPDATE_SERVER + "/updates/";  // Base URL for updates
const std::string VERSION_CHECK_URL = UPDATE_SERVER + "/version.txt";  // URL to check latest version

// Function to play the MP3 file in a separate thread
void PlayMusicThread(const std::string& filePath) {
    // Convert to wide string for MCI
    std::wstring wFilePath(filePath.begin(), filePath.end());

    // Set up MCI command strings
    std::wstring openCommand = L"open \"" + wFilePath + L"\" type mpegvideo alias music";
    std::wstring playCommand = L"play music";

    // Buffer for error messages
    wchar_t errorMsg[256];

    // Open the MP3 file
    DWORD openError = mciSendStringW(openCommand.c_str(), NULL, 0, NULL);
    if (openError != 0) {
        mciGetErrorStringW(openError, errorMsg, 256);
        Log::Error(std::string("Failed to open music file: ") +
            std::string(errorMsg, errorMsg + wcslen(errorMsg)));
        g_musicPlaying = false;
        return;
    }

    // Set volume (0-1000)
    mciSendStringW(L"setaudio music volume to 500", NULL, 0, NULL);

    // Play the file
    DWORD playError = mciSendStringW(playCommand.c_str(), NULL, 0, NULL);
    if (playError != 0) {
        mciGetErrorStringW(playError, errorMsg, 256);
        Log::Error(std::string("Failed to play music file: ") +
            std::string(errorMsg, errorMsg + wcslen(errorMsg)));
        mciSendStringW(L"close music", NULL, 0, NULL);
        g_musicPlaying = false;
        return;
    }

    g_musicPlaying = true;

    // Check status in a loop until the music should be stopped
    while (g_musicPlaying) {
        Sleep(100);  // Check status every 100ms
    }

    // Close the music when the loop exits
    mciSendStringW(L"close music", NULL, 0, NULL);
}

// Function to download MP3 file and save it locally
bool DownloadMusicFile(const std::string& url, const std::string& localFilePath) {
    bool success = false;
    HINTERNET hInternet = InternetOpenA("HuhtalaHook Music Downloader", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);

    if (!hInternet) {
        DWORD error = GetLastError();
        Log::Warning("InternetOpen failed with error code: " + std::to_string(error), false);
        return false;
    }

    // Set timeout values (30 seconds)
    DWORD timeout = 30000;
    InternetSetOption(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOption(hInternet, INTERNET_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOption(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

    Log::Info("Downloading music file...");
    HINTERNET hConnect = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);

    if (!hConnect) {
        DWORD error = GetLastError();
        Log::Warning("InternetOpenUrl failed with error code: " + std::to_string(error), false);
        InternetCloseHandle(hInternet);
        return false;
    }

    // Check HTTP status code
    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    if (HttpQueryInfoA(hConnect, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &statusCodeSize, NULL)) {
        if (statusCode != 200) {
            Log::Warning("HTTP Error: " + std::to_string(statusCode), false);
            InternetCloseHandle(hConnect);
            InternetCloseHandle(hInternet);
            return false;
        }
    }

    // Create the file to save data
    std::ofstream outputFile(localFilePath, std::ios::binary);
    if (!outputFile) {
        Log::Error("Failed to create output file: " + localFilePath);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
    }

    // Download and save the file
    char buffer[8192];
    DWORD bytesRead;
    DWORD totalBytesRead = 0;

    while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        outputFile.write(buffer, bytesRead);
        totalBytesRead += bytesRead;
    }

    outputFile.close();
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    if (totalBytesRead > 0) {
        Log::PreviousLine();
        Log::Fine("Music file downloaded successfully (" + std::to_string(totalBytesRead) + " bytes)");
        success = true;
    }
    else {
        Log::PreviousLine();
        Log::Error("Failed to download music file (0 bytes received)");
        // Delete empty file
        std::filesystem::remove(localFilePath);
        success = false;
    }

    return success;
}

// Main function to handle music playback
bool PlayBackgroundMusic(const std::string& musicFileName) {
    // Get music directory path
    char documentsPath[MAX_PATH];
    if (SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, 0, documentsPath) != S_OK) {
        Log::Error("Failed to get the Documents folder path");
        return false;
    }

    std::string musicDir = std::string(documentsPath) + "\\HuhtalaHook\\Music";

    // Create music directory if it doesn't exist
    if (!std::filesystem::exists(musicDir)) {
        try {
            std::filesystem::create_directories(musicDir);
            Log::Fine("Created music directory: " + musicDir);
        }
        catch (const std::filesystem::filesystem_error& e) {
            Log::Error("Failed to create music directory: " + std::string(e.what()));
            return false;
        }
    }

    // Full path to music file
    std::string musicFilePath = musicDir + "\\" + musicFileName;
    g_musicPath = musicFilePath;

    Log::Info("Setting up background music...");

    // Check if music file already exists
    bool needDownload = true;
    if (std::filesystem::exists(musicFilePath)) {
        // File exists, check if it's a valid MP3 (size > 0)
        uintmax_t fileSize = std::filesystem::file_size(musicFilePath);
        if (fileSize > 0) {
            Log::PreviousLine();
            Log::Fine("Music file already exists, using cached version");
            needDownload = false;
        }
        else {
            // Empty file, delete it and re-download
            std::filesystem::remove(musicFilePath);
        }
    }

    // Download the file if needed
    if (needDownload) {
        // Download from the same server used for updates
        std::string musicUrl = "http://51.83.133.131/updates/music.mp3";
        if (!DownloadMusicFile(musicUrl, musicFilePath)) {
            Log::Error("Failed to download music file");
            return false;
        }
    }

    // Start playing music in a separate thread
    g_musicThread = std::thread(PlayMusicThread, musicFilePath);
    g_musicThread.detach();  // Detach thread so it continues playing

    // Wait briefly to ensure music starts
    Sleep(500);

    if (g_musicPlaying) {
        Log::Fine("Background music started");
        return true;
    }
    else {
        Log::Warning("Music may not be playing correctly", false);
        return false;
    }
}

// Function to stop music playback (call when application exits)
void StopBackgroundMusic() {
    if (g_musicPlaying) {
        Log::Info("Stopping background music...");

        // Try multiple approaches to ensure the music stops
        // First try standard close
        DWORD mciError = mciSendStringW(L"close music", NULL, 0, NULL);

        // If that failed, try stopping first, then closing
        if (mciError != 0) {
            mciSendStringW(L"stop music", NULL, 0, NULL);
            mciSendStringW(L"close music", NULL, 0, NULL);
        }

        // For extra measure, try closing all MCI devices
        mciSendStringW(L"close all", NULL, 0, NULL);

        g_musicPlaying = false;
        Log::PreviousLine();
        Log::Fine("Background music stopped");
    }
}

// Standard main function
int main(int argc, char* argv[])
{
    // Start background music before anything else
    PlayBackgroundMusic("music.mp3");

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

    // Stop background music before exiting
    StopBackgroundMusic();

    return 0;
}

// Windows-specific entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    return main(0, nullptr);
}

// Authentication function to separate concerns
bool AuthenticateWithKeygen()
{
    // Replace these with your actual Keygen.sh credentials
    std::string account_id = "3dc8a372-5f95-419b-a0c8-238fb7422b07";  // Your account ID
    std::string product_id = "19aff6c0-7c4e-409c-b037-f751e3cab041";  // Your product ID
    std::string api_key = "";  // Leave empty for public validation

    // Create KeygenAuth instance
    KeygenAuth keygenAuth(account_id, product_id, api_key);

    Log::Info("Initializing Keygen.sh authentication");
    bool initSuccess = keygenAuth.Init();

    if (!initSuccess)
    {
        Log::PreviousLine();
        Log::Error("Auth system initialization failed: " + keygenAuth.GetMessage());
        Sleep(3000);
        return false;
    }

    Log::PreviousLine();
    Log::Fine("Auth system initialized successfully");

    // Get documents path for config and license storage
    char documentsPath[MAX_PATH];
    if (SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, 0, documentsPath) != S_OK)
    {
        Log::Error("Failed to get the Documents folder path");
        return false;
    }

    MenuConfig::path = documentsPath;
    MenuConfig::docPath = documentsPath;
    MenuConfig::path += "\\HuhtalaHook";

    // Create directory if it doesn't exist
    if (!std::filesystem::exists(MenuConfig::path)) {
        std::filesystem::create_directory(MenuConfig::path);
    }

    // Check if license file exists
    std::string licensePath = MenuConfig::path + "\\license.key";
    std::string licenseKey;
    bool hasValidLicense = false;

    // Try to read existing license
    if (std::filesystem::exists(licensePath))
    {
        std::ifstream licenseFile(licensePath);
        if (licenseFile.is_open())
        {
            std::getline(licenseFile, licenseKey);
            licenseFile.close();

            // Verify saved license
            Log::Info("Verifying saved license");
            hasValidLicense = keygenAuth.ValidateLicense(licenseKey);

            if (hasValidLicense)
            {
                Log::PreviousLine();
                Log::Fine("License verified successfully");
            }
            else
            {
                Log::PreviousLine();
                Log::Warning("Saved license invalid: " + keygenAuth.GetMessage(), false);
            }
        }
    }

    // If no valid license found, ask user for key
    if (!hasValidLicense)
    {
        int attempts = 0;
        const int maxAttempts = 3;

        while (!hasValidLicense && attempts < maxAttempts)
        {
            Log::Info("Please enter your license key:");
            std::cout << "> ";
            std::cin >> licenseKey;

            // Verify the license
            Log::Info("Verifying license");
            hasValidLicense = keygenAuth.ValidateLicense(licenseKey);

            if (hasValidLicense)
            {
                Log::PreviousLine();
                Log::Fine("License verified successfully");

                // Save valid license
                std::ofstream licenseFile(licensePath);
                if (licenseFile.is_open())
                {
                    licenseFile << licenseKey;
                    licenseFile.close();
                }
            }
            else
            {
                Log::PreviousLine();
                Log::Error("Invalid license: " + keygenAuth.GetMessage());
                attempts++;

                if (attempts >= maxAttempts)
                {
                    Log::Error("Too many failed attempts. Exiting...");
                    Sleep(3000);
                    return false;
                }
            }
        }
    }

    return true;
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

void Cheat()
{
    ShowWindow(GetConsoleWindow(), SW_SHOWNORMAL);
    SetConsoleTitle(L"HuhtalaHook");
    //Init::Verify::RandTitle();

    Log::Custom(R"LOGO(                 
 _   _       _     _        _       _   _             _    
| | | |_   _| |__ | |_ __ _| | __ _| | | | ___   ___ | | __
| |_| | | | | '_ \| __/ _` | |/ _` | |_| |/ _ \ / _ \| |/ /
|  _  | |_| | | | | || (_| | | (_| |  _  | (_) | (_) |   < 
|_| |_|\__,_|_| |_|\__\__,_|_|\__,_|_| |_|\___/ \___/|_|\_\
HH ON TOP!

Version: )LOGO" + GetCurrentVersion(), 13);

    // Check for updates before authentication
    CheckForUpdates();

    // Authenticate with Keygen.sh
    if (!AuthenticateWithKeygen())
    {
        // Authentication failed, exit the application
        return;
    }

    // Continue with the regular initialization
    if (!Init::Verify::CheckWindowVersion())
    {
        Log::Warning("Your OS is unsupported, bugs may occur", true);
    }

#ifndef DBDEBUG
    Log::Info("Checking cheat version");
    switch (Init::Verify::CheckCheatVersion())
    {
    case 0:
        Log::PreviousLine();
        Log::Error("Bad internet connection");
        break;

    case 1:
        Log::PreviousLine();
        Log::Error("Failed to get currently supported versions");
        break;

    case 2:
        Log::PreviousLine();
        Log::Error("Your cheat version is out of support");
        break;

    case 3:
        Log::PreviousLine();
        Log::Fine("Your cheat version is up to date and supported");
        break;

    default:
        Log::PreviousLine();
        Log::Error("Unknown connection error");
        break;
    }
#endif

    Log::Info("Updating offsets");
    switch (Offset.UpdateOffsets())
    {
    case 0:
        Log::PreviousLine();
        Log::Error("Bad internet connection");
        break;

    case 1:
        Log::PreviousLine();
        Log::Error("Failed to UpdateOffsets");
        break;

    case 2:
        Log::PreviousLine();
        Log::Fine("Offsets updated");
        break;

    default:
        Log::PreviousLine();
        Log::Error("Unknown connection error");
        break;
    }

    Log::Info("Connecting to kernel mode driver");
    if (memoryManager.ConnectDriver(L"\\\\.\\DragonBurn-kernel"))
    {
        Log::PreviousLine();
        Log::Fine("Successfully connected to kernel mode driver");
    }
    else
    {
        Log::PreviousLine();
        Log::Error("Failed to connect to kernel mode driver");
    }

    std::cout << '\n';
    bool preStart = false;
    while (memoryManager.GetProcessID(L"cs2.exe") == 0)
    {
        Log::PreviousLine();
        Log::Info("Waiting for CS2");
        preStart = true;
    }

    if (preStart)
    {
        Log::PreviousLine();
        Log::Info("Connecting to CS2(it may take some time)");
        Sleep(20000);
    }

    Log::PreviousLine();
    Log::Fine("Connected to CS2");
    Log::Info("Linking to CS2");

#ifndef DBDEBUG
    switch (Init::Client::CheckCS2Version())
    {
    case 0:
        Log::PreviousLine();
        Log::Error("Failed to get the current game version");
        break;

    case 1:
        Log::PreviousLine();
        Log::Warning("Offsets are outdated, we'll update them asap. With current offsets, cheat may work unstable", true);
        break;

    case 2:
        Log::PreviousLine();
        Log::Error("Failed to get cloud version");
        break;

    case 3:
        break;

    default:
        Log::PreviousLine();
        Log::Error("Failed to get the current game version");
        break;
    }
#endif

    if (!memoryManager.Attach(memoryManager.GetProcessID(L"cs2.exe")))
    {
        Log::PreviousLine();
        Log::Error("Failed to attach to the process");
    }

    if (!gGame.InitAddress())
    {
        Log::PreviousLine();
        Log::Error("Failed to Init Address");
    }

    Log::PreviousLine();
    Log::Fine("Linked to CS2");

    if (fs::exists(MenuConfig::docPath + "\\Adobe Software Data"))
    {
        fs::rename(MenuConfig::docPath + "\\Adobe Software Data", MenuConfig::path);
    }

    if (fs::exists(MenuConfig::path))
    {
        Log::Fine("Config folder connected: " + MenuConfig::path);
    }
    else
    {
        if (fs::create_directory(MenuConfig::path))
        {
            Log::Fine("Config folder connected: " + MenuConfig::path);
        }
        else
        {
            Log::Error("Failed to create the config directory");
        }
    }

    if (fs::exists(MenuConfig::path + "\\default.cfg"))
        MenuConfig::defaultConfig = true;

    // Stop background music with a forced approach before showing loaded message
    Log::Info("Finalizing initialization...");
    StopBackgroundMusic();

    // Extra measure - wait a brief moment to ensure music has stopped
    Sleep(500);

    Log::PreviousLine();
    Log::Fine("HuhtalaHook loaded");

#ifndef DBDEBUG
    Sleep(3000);
    ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif

    try
    {
        Gui.AttachAnotherWindow("Counter-Strike 2", "SDL_app", Cheats::Run);
    }
    catch (OSImGui::OSException& e)
    {
        Log::Error(e.what());
    }
}
