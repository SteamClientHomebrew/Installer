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

#pragma once
#include <string>
#include <windows.h>
#include <iostream>
#include <shellapi.h>
#include <fmt/core.h>
#include <optional>
#include <filesystem>
#include <shobjidl.h>
#include <tlhelp32.h>
#include "components.h"

inline std::string BytesToReadableFormat(float bytes)
{
    const char* suffixes[] = { "B", "KB", "MB", "GB", "TB" };
    int suffixIndex = 0;

    while (bytes >= 1024 && suffixIndex < 4) {
        bytes /= 1024;
        suffixIndex++;
    }

    char buffer[32];
    sprintf(buffer, "%.2f %s", bytes, suffixes[suffixIndex]);

    return std::string(buffer);
}

enum ColorScheme
{
    Dark,
    Light
};

static ColorScheme GetWindowsColorScheme()
{
    HKEY key;
    DWORD value;
    DWORD size = sizeof(DWORD);

    if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_READ, &key) != ERROR_SUCCESS) {
        return Dark;
    }

    if (RegQueryValueEx(key, "SystemUsesLightTheme", nullptr, nullptr, (LPBYTE)&value, &size) != ERROR_SUCCESS) {
        RegCloseKey(key);
        return Dark;
    }

    RegCloseKey(key);
    return value == 1 ? Light : Dark;
}

static std::string SanitizeDirectoryName(const std::string& path)
{
    std::wstring widePath(path.begin(), path.end());

    size_t startPos = 0;
    if (widePath.length() >= 2 && widePath[1] == L':') {
        widePath[0] = std::towupper(widePath[0]);
        startPos = 2;
    }

    std::wstring result = (startPos == 2) ? widePath.substr(0, 2) : L"";

    size_t pos = startPos;
    while (pos < widePath.length()) {
        size_t delimPos = widePath.find_first_of(L"/\\", pos);
        if (delimPos == std::wstring::npos) {
            delimPos = widePath.length();
        }

        std::wstring component = widePath.substr(pos, delimPos - pos);
        if (!component.empty()) {
            if (!result.empty() && result.back() != L'\\') {
                result += L'\\';
            }

            WIN32_FIND_DATAW findData;
            std::wstring searchPath = result.empty() ? component : result + component;
            HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);

            if (hFind != INVALID_HANDLE_VALUE) {
                result += findData.cFileName;
                FindClose(hFind);
            } else {
                result += component;
            }
        }
        pos = delimPos + 1;
    }

    std::string correctPath(result.begin(), result.end());
    return correctPath;
}

static std::string GetSteamPath()
{
    HKEY key;
    char value[260];
    DWORD size = sizeof(value);

    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Valve\\Steam", 0, KEY_READ, &key) != ERROR_SUCCESS) {
        return {};
    }

    if (RegQueryValueExA(key, "SteamPath", nullptr, nullptr, (LPBYTE)value, &size) != ERROR_SUCCESS) {
        RegCloseKey(key);
        return {};
    }

    RegCloseKey(key);
    return SanitizeDirectoryName(std::string(value, size - 1)); // Remove null terminator
}

static const void OpenUrl(const char* url)
{
#ifdef _WIN32
    ShellExecuteA(nullptr, "open", url, nullptr, nullptr, SW_SHOWNORMAL);
#elif __linux__
    system(fmt::format("xdg-open {}", url).c_str());
#endif
}

static std::string OpenFolderDialog()
{
    const auto WStringToString = [](const std::wstring& wstr) -> std::string
    {
        if (wstr.empty()) {
            return {};
        }

        size_t size_needed = std::wcstombs(nullptr, wstr.c_str(), 0) + 1;
        std::string str(size_needed, 0);
        std::wcstombs(&str[0], wstr.c_str(), size_needed);
        str.pop_back();

        return str;
    };

    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) {
        return {};
    }

    IFileOpenDialog* pFileOpen = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen)))) {
        CoUninitialize();
        return {};
    }

    unsigned long options;
    if (SUCCEEDED(pFileOpen->GetOptions(&options))) {
        pFileOpen->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
    }

    if (FAILED(pFileOpen->Show(nullptr))) {
        pFileOpen->Release();
        CoUninitialize();
        return {};
    }

    IShellItem* pItem = nullptr;
    if (FAILED(pFileOpen->GetResult(&pItem))) {
        pFileOpen->Release();
        CoUninitialize();
        return {};
    }

    wchar_t* folderPath = nullptr;
    std::string result;
    if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &folderPath))) {
        if (folderPath) {
            result = WStringToString(folderPath);
            CoTaskMemFree(folderPath);
        }
    }

    pItem->Release();
    pFileOpen->Release();
    CoUninitialize();

    return result;
}

static std::optional<std::string> SelectNewSteamPath()
{
    const auto steamPath = std::filesystem::path(OpenFolderDialog());

    if (steamPath.empty()) {
        return {};
    }

    if (!std::filesystem::exists(steamPath / "steam.exe")) {
        MessageBoxA(nullptr, "Invalid Steam path selected", "Error", MB_ICONERROR);
        return {};
    }

    return steamPath.string();
}

static std::string ToTimeAgo(const std::string& isoTimestamp)
{
    std::tm tm = {};
    std::istringstream ss(isoTimestamp);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ"); // UTC time format

    if (ss.fail()) {
        return "Invalid timestamp";
    }

    // Convert to time_point
    std::time_t inputTime = std::mktime(&tm);
    auto inputTimePoint = std::chrono::system_clock::from_time_t(inputTime);
    auto now = std::chrono::system_clock::now();

    // Calculate difference in seconds
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - inputTimePoint).count();

    // Helper lambda to handle singular/plural formatting
    auto formatTime = [](long count, const char* unit) { return fmt::format("{} {}{}", count, unit, (count == 1 ? "" : "s")); };

    if (diff < 60)
        return "just now";
    if (diff < 3600)
        return formatTime(diff / 60, "minute") + " ago";
    if (diff < 86400)
        return formatTime(diff / 3600, "hour") + " ago";
    if (diff < 2592000)
        return formatTime(diff / 86400, "day") + " ago";
    if (diff < 31536000)
        return fmt::format("about {} ago", formatTime(diff / 2592000, "month"));

    return fmt::format("about {} ago", formatTime(diff / 31536000, "year"));
}

static void StartSteamFromPath(std::string steamPath)
{
    std::string steamExePath = (std::filesystem::path(steamPath) / "steam.exe").string();

    STARTUPINFO si = { 0 };
    PROCESS_INFORMATION pi = { 0 };

    si.cb = sizeof(si);

    char cmd[MAX_PATH];
    strcpy_s(cmd, steamExePath.c_str());

    if (!CreateProcess(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        ShowMessageBox("Whoops!", fmt::format("Failed to start Steam. Please check your installation path. Error code: {}", GetLastError()), Error);
        return;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    std::exit(0);
}

static bool KillSteamProcess()
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hSnapshot == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create snapshot. Error: " << GetLastError() << std::endl;
        return false;
    }

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe)) {
        do {
            if (_stricmp(pe.szExeFile, "steam.exe") == 0) {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                if (hProcess) {
                    if (TerminateProcess(hProcess, 0)) {
                        CloseHandle(hProcess);
                        CloseHandle(hSnapshot);
                        return true;
                    } else {
                        ShowMessageBox("Whoops!", fmt::format("Failed to terminate process. Error: {}", GetLastError()), Error);
                    }
                    CloseHandle(hProcess);
                } else {
                    ShowMessageBox("Whoops!", fmt::format("Failed to open process. Error: {}", GetLastError()), Error);
                }
            }
        } while (Process32Next(hSnapshot, &pe));
    } else {
        ShowMessageBox("Whoops!", fmt::format("Failed to retrieve process list. Error: {}", GetLastError()), Error);
    }

    CloseHandle(hSnapshot);
    return false;
}

static uint64_t GetFolderSize(const std::filesystem::path& path, std::error_code& ec)
{
    if (!std::filesystem::exists(path, ec)) {
        return 0;
    }

    std::error_code file_ec;
    auto metadata = std::filesystem::status(path, file_ec);
    if (file_ec) {
        return 0;
    }

    if (std::filesystem::is_regular_file(metadata)) {
        return std::filesystem::file_size(path, file_ec);
    }

    uint64_t size = 0;
    for (const auto& entry : std::filesystem::directory_iterator(path, std::filesystem::directory_options::skip_permission_denied, ec)) {
        if (ec) {
            continue;
        }

        auto entry_metadata = std::filesystem::status(entry, file_ec);
        if (file_ec) {
            continue;
        }

        if (std::filesystem::is_directory(entry_metadata)) {
            size += GetFolderSize(entry.path(), ec);
        } else if (std::filesystem::is_regular_file(entry_metadata)) {
            size += std::filesystem::file_size(entry, file_ec);
        }
    }
    return size;
}