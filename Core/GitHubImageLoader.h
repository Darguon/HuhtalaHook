#pragma once
#include <Windows.h>
#include <WinInet.h>
#include <string>
#include <vector>
#include <d3d11.h>

#pragma comment(lib, "WinInet.lib")

// Simple GitHub image loader class
class GitHubImageLoader
{
private:
    std::string m_Username;
    std::string m_Repository;
    std::string m_Branch;
    std::string m_ImageDirectory;

public:
    // Constructor - initialize with GitHub repository information
    GitHubImageLoader(const std::string& username,
        const std::string& repo,
        const std::string& branch = "main",
        const std::string& imageDir = "Assets/")
        : m_Username(username)
        , m_Repository(repo)
        , m_Branch(branch)
        , m_ImageDirectory(imageDir)
    {
    }

    // Generate GitHub raw content URL for a file
    std::string GetURL(const std::string& filename) const
    {
        return "https://raw.githubusercontent.com/" +
            m_Username + "/" +
            m_Repository + "/" +
            m_Branch + "/" +
            m_ImageDirectory +
            filename;
    }

    // Download image data from GitHub
    bool DownloadImage(const std::string& filename, std::vector<BYTE>& data)
    {
        data.clear();
        std::string url = GetURL(filename);

        // Open internet connection
        HINTERNET hInternet = InternetOpenA("GitHub Image Loader", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (!hInternet)
            return false;

        // Open URL
        HINTERNET hConnect = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
        if (!hConnect)
        {
            InternetCloseHandle(hInternet);
            return false;
        }

        // Read data
        const int bufferSize = 8192;
        BYTE buffer[bufferSize];
        DWORD bytesRead;

        while (InternetReadFile(hConnect, buffer, bufferSize, &bytesRead) && bytesRead > 0)
        {
            data.insert(data.end(), buffer, buffer + bytesRead);
        }

        // Clean up
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);

        return !data.empty();
    }

    // Load texture directly from GitHub
    bool LoadTexture(const std::string& filename, ID3D11ShaderResourceView** texture,
        int* width, int* height)
    {
        std::vector<BYTE> imageData;
        if (!DownloadImage(filename, imageData))
            return false;

        // Use your existing texture creation function
        return Gui.LoadTextureFromMemory(imageData.data(), imageData.size(), texture, width, height);
    }
};