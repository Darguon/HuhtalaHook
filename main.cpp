#include "Core/Cheats.h"
#include "Offsets/Offsets.h"
#include "Resources/Language.hpp"
#include "Core/Init.h"
#include "Config/ConfigSaver.h"
#include "Helpers/Logger.h"
#include <filesystem>
#include <KnownFolders.h>
#include <ShlObj.h>
#include "SimpleAuth.h"  // Include our self-contained Auth header
#include "xorstr.hpp"

using namespace std;

namespace fs = filesystem;
string fileName;

void Cheat();

// Standard main function
int main(int argc, char* argv[])
{
    Cheat();
    return 0;
}

// Windows-specific entry point (just in case)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    return main(0, nullptr);
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
Panisin Huhtalaa jos vain voisin ;(


)LOGO", 13);

    // Initialize Auth
    std::string name = XorStr("HuhtalaHook").c_str();
    std::string ownerid = XorStr("eYuF8QPas4").c_str();
    std::string secret = XorStr("fdcdc9b6ced5890a78d7b055c83f77fddecc1d52d1c070a4f91f20f75be3daa3").c_str();
    std::string version = XorStr("1.0").c_str();
    std::string url = XorStr("https://keyauth.win/api/1.3/").c_str();

    // Create SimpleAuth instance
    SimpleAuth authSystem(name, ownerid, secret, version, url);

    Log::Info("Initializing auth system");
    bool initSuccess = authSystem.Init();

    if (!initSuccess)
    {
        Log::PreviousLine();
        Log::Error("Auth system initialization failed: " + authSystem.GetMessage());
        Sleep(3000);
        exit(0);
    }

    Log::PreviousLine();
    Log::Fine("Auth system initialized successfully");

    // Get documents path for config and license storage
    char documentsPath[MAX_PATH];
    if (SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, 0, documentsPath) != S_OK)
        Log::Error("Failed to get the Documents folder path");

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
            hasValidLicense = authSystem.VerifyLicense(licenseKey);

            if (hasValidLicense)
            {
                Log::PreviousLine();
                Log::Fine("License verified successfully");
            }
            else
            {
                Log::PreviousLine();
                Log::Warning("Saved license invalid: " + authSystem.GetMessage(), false);
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
            hasValidLicense = authSystem.VerifyLicense(licenseKey);

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
                Log::Error("Invalid license: " + authSystem.GetMessage());
                attempts++;

                if (attempts >= maxAttempts)
                {
                    Log::Error("Too many failed attempts. Exiting...");
                    Sleep(3000);
                    exit(0);
                }
            }
        }
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
