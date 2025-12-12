/**
 * ==================================================
 *   _____ _ _ _             _
 *  |     |_| | |___ ___ ___|_|_ _ _____
 *  | | | | | | | -_|   |   | | | |     |
 *  |_|_|_|_|_|_|___|_|_|_|_|_|___|_|_|_|
 *
 * ==================================================
 *
 * Copyright (c) 2025 Project Millennium
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <string>
#include <filesystem>
#include <http.h>
#include <nlohmann/json.hpp>
#include <semver.h>
#include <windows.h>
#include <iostream>

namespace fs = std::filesystem;

/**
 * Strip leading 'v' or 'V' from version string for semver comparison
 */
static std::string NormalizeVersion(const std::string& version)
{
    if (!version.empty() && (version[0] == 'v' || version[0] == 'V')) {
        return version.substr(1);
    }
    return version;
}

/**
 * Get the path of the current executable
 */
static fs::path GetCurrentExecutablePath()
{
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    return fs::path(path);
}

/**
 * Check for updates from GitHub and perform self-update if available
 */
void CheckForAndDownloadUpdates()
{
    const std::string currentVersion = MILLENNIUM_VERSION;
    const std::string apiUrl = "https://api.github.com/repos/SteamClientHomebrew/Installer/releases";

    // Fetch releases from GitHub API
    std::string response = Http::Get(apiUrl.c_str(), false);
    if (response.empty()) {
        std::cerr << "Failed to fetch update information from GitHub." << std::endl;
        return;
    }

    nlohmann::json releases;
    try {
        releases = nlohmann::json::parse(response);
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "Failed to parse GitHub API response: " << e.what() << std::endl;
        return;
    }

    if (!releases.is_array() || releases.empty()) {
        std::cerr << "No releases found." << std::endl;
        return;
    }

    // Get the latest release (first in the array, as GitHub returns them sorted by date)
    const auto& latestRelease = releases[0];
    if (!latestRelease.contains("tag_name")) {
        std::cerr << "Invalid release format." << std::endl;
        return;
    }

    std::string latestVersion = latestRelease["tag_name"].get<std::string>();

    // Compare versions (normalize to strip 'v' prefix)
    try {
        if (Semver::Compare(NormalizeVersion(currentVersion), NormalizeVersion(latestVersion)) >= 0) {
            std::cout << "Installer is up to date (current: " << currentVersion << ", latest: " << latestVersion << ")." << std::endl;
            return;
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to compare versions: " << e.what() << std::endl;
        return;
    }

    std::cout << "Update available: " << currentVersion << " -> " << latestVersion << std::endl;

    // Find the Windows executable asset
    std::string downloadUrl;
    if (latestRelease.contains("assets") && latestRelease["assets"].is_array()) {
        for (const auto& asset : latestRelease["assets"]) {
            if (!asset.contains("name") || !asset.contains("browser_download_url")) {
                continue;
            }
            std::string assetName = asset["name"].get<std::string>();
            // Look for the Windows installer executable
            if (assetName.find(".exe") != std::string::npos) {
                downloadUrl = asset["browser_download_url"].get<std::string>();
                break;
            }
        }
    }

    if (downloadUrl.empty()) {
        std::cerr << "Could not find installer executable in latest release." << std::endl;
        return;
    }

    // Get paths
    fs::path currentExePath = GetCurrentExecutablePath();
    fs::path tempDir = fs::temp_directory_path() / "MillenniumInstallerUpdate";
    fs::path oldExePath = tempDir / ("old_" + currentExePath.filename().string());
    fs::path newExePath = currentExePath;

    // Create temp directory if it doesn't exist
    try {
        fs::create_directories(tempDir);
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Failed to create temp directory: " << e.what() << std::endl;
        return;
    }

    // Move current installer to temp folder
    try {
        if (fs::exists(oldExePath)) {
            fs::remove(oldExePath);
        }
        fs::rename(currentExePath, oldExePath);
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Failed to move current installer to temp folder: " << e.what() << std::endl;
        return;
    }

    // Download the new version
    std::cout << "Downloading update from: " << downloadUrl << std::endl;
    bool downloadSuccess = Http::downloadFile(downloadUrl, newExePath.string(), 0, nullptr, false);

    if (!downloadSuccess) {
        std::cerr << "Failed to download update. Restoring old version..." << std::endl;
        // Restore old version
        try {
            fs::rename(oldExePath, currentExePath);
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Failed to restore old version: " << e.what() << std::endl;
        }
        return;
    }

    std::cout << "Update downloaded successfully. Launching new version..." << std::endl;

    // Launch the new version
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    std::wstring newExePathW = newExePath.wstring();
    if (CreateProcessW(newExePathW.c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        // Exit the current process
        ExitProcess(0);
    } else {
        std::cerr << "Failed to launch new version. Error: " << GetLastError() << std::endl;
        // Restore old version since we couldn't launch the new one
        try {
            fs::remove(newExePath);
            fs::rename(oldExePath, currentExePath);
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Failed to restore old version: " << e.what() << std::endl;
        }
    }
}