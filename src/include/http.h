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
#include <unordered_map>
#include <curl/curl.h>
#include <iostream>
#include <format>
#include <nlohmann/json.hpp>
#include "components.h"

static size_t WriteByteCallback(char* ptr, size_t size, size_t nmemb, std::string* data)
{
    data->append(ptr, size * nmemb);
    return size * nmemb;
}

static size_t HeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata)
{
    size_t totalSize = size * nitems;
    auto* headers = static_cast<std::unordered_map<std::string, std::string>*>(userdata);

    std::string line(buffer, totalSize);

    auto colonPos = line.find(':');
    if (colonPos != std::string::npos) {
        std::string key = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1);

        // Lowercase the key for case-insensitive lookup
        for (auto& c : key) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        // Trim whitespace from value
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t\r\n") + 1);

        (*headers)[key] = value;
    }

    return totalSize;
}

namespace Http
{

struct Response
{
    std::string body;
    long statusCode = 0;
    CURLcode curlCode = CURLE_OK;
    std::unordered_map<std::string, std::string> headers;

    bool ok() const
    {
        return curlCode == CURLE_OK && statusCode >= 200 && statusCode < 300;
    }
    bool isRateLimited() const
    {
        return statusCode == 429 || (statusCode == 403 && body.find("rate limit") != std::string::npos);
    }
    bool isNotFound() const
    {
        return statusCode == 404;
    }
    bool isNetworkError() const
    {
        return curlCode != CURLE_OK;
    }

    std::string networkErrorReason() const
    {
        switch (curlCode) {
        case CURLE_COULDNT_RESOLVE_HOST:
            return "DNS resolution failed — check your internet connection or DNS settings.";
        case CURLE_COULDNT_RESOLVE_PROXY:
            return "Could not resolve proxy — check your proxy configuration.";
        case CURLE_COULDNT_CONNECT:
            return "Connection refused — GitHub may be down, or a firewall/proxy is blocking the connection.";
        case CURLE_OPERATION_TIMEDOUT:
            return "Connection timed out — your internet may be slow or GitHub is unreachable.";
        case CURLE_SSL_CONNECT_ERROR:
        case CURLE_SSL_CERTPROBLEM:
        case CURLE_SSL_CIPHER:
        case CURLE_PEER_FAILED_VERIFICATION:
        case CURLE_SSL_PINNEDPUBKEYNOTMATCH:
            return "SSL/TLS error — a corporate proxy, antivirus, or misconfigured network may be interfering with secure connections.";
        case CURLE_TOO_MANY_REDIRECTS:
            return "Too many redirects — a captive portal or proxy may be intercepting the request.";
        case CURLE_SEND_ERROR:
        case CURLE_RECV_ERROR:
            return "Connection was interrupted — check your network stability.";
        case CURLE_FAILED_INIT:
            return "Failed to initialize the HTTP client.";
        default:
            return std::string("Network error: ") + curl_easy_strerror(curlCode);
        }
    }

    std::string rateLimitMessage() const
    {
        std::string msg;
        if (statusCode == 429) {
            msg = "Too many requests to the GitHub API.";
        } else {
            msg = "GitHub API rate limit exceeded.";
        }

        // Check for x-ratelimit-reset header to show retry time
        auto it = headers.find("x-ratelimit-reset");
        if (it != headers.end()) {
            try {
                long resetTime = std::stol(it->second);
                auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                long secondsLeft = resetTime - static_cast<long>(now);

                if (secondsLeft > 0) {
                    long minutes = (secondsLeft + 59) / 60;
                    msg += std::format("\n\nYou can try again in {} {}.", minutes, minutes == 1 ? "minute" : "minutes");
                } else {
                    msg += "\n\nThe rate limit should have reset — try again now.";
                }
            } catch (...) {
                msg += "\n\nPlease wait a few minutes and try again.";
            }
        } else {
            msg += "\n\nPlease wait a few minutes and try again.";
        }

        msg += "\n\nGitHub allows 60 requests per hour for unauthenticated users.";
        return msg;
    }

    std::string httpErrorMessage() const
    {
        // Try to extract GitHub's error message from the JSON response body
        try {
            auto json = nlohmann::json::parse(body);
            if (json.contains("message") && json["message"].is_string()) {
                return std::format("GitHub API error (HTTP {}): {}", statusCode, json["message"].get<std::string>());
            }
        } catch (...) {}

        return std::format("Failed to fetch version information (HTTP {}). Please try again later.", statusCode);
    }
};

static Response GetEx(const char* url, int maxRetries = 3, int timeoutSeconds = 30)
{
    Response result;
    CURL* curl = curl_easy_init();

    if (!curl) {
        result.curlCode = CURLE_FAILED_INIT;
        return result;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteByteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result.body);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &result.headers);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "starlight/1.0");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(timeoutSeconds));
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);

    int attempts = 0;
    while (attempts < maxRetries) {
        result.body.clear();
        result.curlCode = curl_easy_perform(curl);

        if (result.curlCode == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result.statusCode);
            break;
        }

        attempts++;
        if (attempts < maxRetries) {
            // Exponential backoff: 100ms, 200ms, 400ms...
            std::this_thread::sleep_for(std::chrono::milliseconds(100 * (1 << (attempts - 1))));
        }
    }

    curl_easy_cleanup(curl);
    return result;
}

// Legacy wrapper for backward compatibility
static std::string Get(const char* url, bool retry = true)
{
    auto response = GetEx(url, retry ? 3 : 1);
    return response.ok() ? response.body : std::string();
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
        ShowMessageBox("Whoops!", std::format("Failed to open file to write Millennium into: '{}'", outputPath), Error);
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

    long httpCode = 0;
    if (res == CURLE_HTTP_RETURNED_ERROR || res == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    }

    curl_easy_cleanup(curl);
    fclose(fp);

    if (res != CURLE_OK) {
        std::string reason;
        switch (res) {
        case CURLE_COULDNT_RESOLVE_HOST:
            reason = "DNS resolution failed — check your internet connection or DNS settings.";
            break;
        case CURLE_COULDNT_CONNECT:
            reason = "Connection refused — the server may be down, or a firewall/proxy is blocking the connection.";
            break;
        case CURLE_OPERATION_TIMEDOUT:
            reason = "Download timed out — your internet may be too slow or the server is unreachable.";
            break;
        case CURLE_SSL_CONNECT_ERROR:
        case CURLE_SSL_CERTPROBLEM:
        case CURLE_SSL_CIPHER:
        case CURLE_PEER_FAILED_VERIFICATION:
            reason = "SSL/TLS error — a corporate proxy, antivirus, or misconfigured network may be interfering.";
            break;
        case CURLE_SEND_ERROR:
        case CURLE_RECV_ERROR:
        case CURLE_PARTIAL_FILE:
            reason = "Download was interrupted — check your network stability and try again.";
            break;
        case CURLE_HTTP_RETURNED_ERROR:
            reason = std::format("Server returned HTTP error {}.", httpCode);
            break;
        default:
            reason = std::string("Network error: ") + curl_easy_strerror(res);
            break;
        }

        ShowMessageBox("Whoops!", std::format("Failed to download file.\n\n{}", reason), Error);
        return false;
    }
    return true;
}
} // namespace Http
