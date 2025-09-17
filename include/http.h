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
#include <chrono>
#include <thread>
#include <curl/curl.h>
#include <iostream>
#include "components.h"
#include <fmt/core.h>

static size_t WriteByteCallback(char* ptr, size_t size, size_t nmemb, std::string* data)
{
    data->append(ptr, size * nmemb);
    return size * nmemb;
}

namespace Http
{
static std::string Get(const char* url, bool retry = true)
{
    CURL* curl;
    CURLcode res;
    std::string response;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteByteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "starlight/1.0");
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Follow redirects

        while (true) {
            res = curl_easy_perform(curl);

            if (!retry || res == CURLE_OK) {
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(3));
        }
        curl_easy_cleanup(curl);
    }
    return response;
}

struct ProgressData
{
    double fileSize;
    std::function<void(double, double)> progressCallback;
    std::chrono::time_point<std::chrono::steady_clock> lastUpdateTime;
    bool showProgress;
};

// File write data structure
struct WriteData
{
    FILE* fp;
};

static size_t writeCallback(char* contents, size_t size, size_t nmemb, void* userp)
{
    size_t realSize = size * nmemb;
    WriteData* writeData = static_cast<WriteData*>(userp);
    return fwrite(contents, size, nmemb, writeData->fp);
}

// Modern progress callback function for libcurl (CURLOPT_XFERINFOFUNCTION)
static int xferInfoCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
    ProgressData* prog = static_cast<ProgressData*>(clientp);

    // Use provided file size if dltotal is 0 (unknown)
    double total = (dltotal > 0) ? static_cast<double>(dltotal) : prog->fileSize;
    double downloaded = static_cast<double>(dlnow);

    // Call the user-provided progress callback
    auto now = std::chrono::steady_clock::now();
    if (prog->showProgress && std::chrono::duration_cast<std::chrono::milliseconds>(now - prog->lastUpdateTime).count() > 100) {
        prog->progressCallback(downloaded, total);
        prog->lastUpdateTime = now;
    }
    return 0; // Return non-zero to abort transfer
}

/**
 * Download a file from a URL to a local path with progress tracking
 *
 * @param url The URL of the file to download
 * @param outputPath Local path where the file should be saved
 * @param fileSize Expected file size (in bytes) if known, otherwise 0
 * @param progressCallback Optional callback to track download progress
 * @param showProgress Whether to show progress (true by default)
 *
 * @return true if download was successful, false otherwise
 */
static bool downloadFile(const std::string& url, const std::string& outputPath, double fileSize = 0, std::function<void(double, double)> progressCallback = nullptr,
                         bool showProgress = true)
{
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize curl" << std::endl;
        ShowMessageBox("Whoops!", "Failed to initialize CURL to download Millennium!", Error);
        return false;
    }

    FILE* fp = fopen(outputPath.c_str(), "wb");
    if (!fp) {
        std::cerr << "Failed to open output file: " << outputPath << std::endl;
        ShowMessageBox("Whoops!", fmt::format("Failed to open file to write Millennium into: '{}'", outputPath), Error);
        curl_easy_cleanup(curl);
        return false;
    }

    WriteData writeData = { fp };

    ProgressData progressData = { fileSize, progressCallback, std::chrono::steady_clock::now(), showProgress };

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &writeData);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, xferInfoCallback);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &progressData);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 0L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");

    CURLcode res = curl_easy_perform(curl);

    curl_easy_cleanup(curl);
    fclose(fp);

    if (res != CURLE_OK) {
        ShowMessageBox("Whoops!", fmt::format("Failed to download file from URL: '{}'", url), Error);
        return false;
    }
    return true;
}
} // namespace Http
