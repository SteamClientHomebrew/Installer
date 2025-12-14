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

#include <imgui.h>
#include <animate.h>
#include <chrono>
#include <texture.h>
#include <imgui_stdlib.h>
#include <imgui_internal.h>
#include <memory>
#include <router.h>
#include <iostream>
#include <dpi.h>
#include <unordered_map>
#include <components.h>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <http.h>
#include <util.h>
#include <imgui_markdown.h>
#include <mini/ini.h>

using namespace ImGui;

const CheckBoxState* checkForUpdates;
const CheckBoxState* automaticallyInstallUpdates;

static ImGui::MarkdownConfig mdConfig;
std::string latestReleaseTag;
std::string installSizeStr;
nlohmann::json releasesList, selectedRelease, osReleaseInfo;

static void UpdateSelectedRelease(const std::string& tag)
{
    if (releasesList.is_null())
        return;

    for (const auto& release : releasesList) {
        if (!release.contains("tag_name"))
            continue;
        if (release["tag_name"].get<std::string>() != tag)
            continue;

        selectedRelease = release;
        installSizeStr.clear();

        for (const auto& asset : selectedRelease["assets"]) {
            std::string assetName = asset["name"];
#ifdef WIN32
            if (assetName == std::format("millennium-{}-windows-x86_64.zip", tag)) {
                osReleaseInfo = asset;
            }
            if (assetName == std::format("millennium-{}-windows-x86_64.installsize", tag)) {
                if (asset.contains("browser_download_url")) {
                    auto sizeResponse = Http::Get(asset["browser_download_url"].get<std::string>().c_str(), false);
                    if (!sizeResponse.empty()) {
                        installSizeStr = sizeResponse;
                        // Trim whitespace
                        installSizeStr.erase(0, installSizeStr.find_first_not_of(" \t\n\r"));
                        installSizeStr.erase(installSizeStr.find_last_not_of(" \t\n\r") + 1);
                    }
                }
            }
#elif __linux__
            if (assetName == std::format("millennium-{}-linux-x86_64.tar.gz", tag)) {
                osReleaseInfo = asset;
            }
            if (assetName == std::format("millennium-{}-linux-x86_64.installsize", tag)) {
                if (asset.contains("browser_download_url")) {
                    auto sizeResponse = Http::Get(asset["browser_download_url"].get<std::string>().c_str(), false);
                    if (!sizeResponse.empty()) {
                        installSizeStr = sizeResponse;
                        installSizeStr.erase(0, installSizeStr.find_first_not_of(" \t\n\r"));
                        installSizeStr.erase(installSizeStr.find_last_not_of(" \t\n\r") + 1);
                    }
                }
            }
#else
            osReleaseInfo = asset;
#endif
        }

        return;
    }
}

const bool FetchVersionInfo()
{
    // fetch all pages of releases from GitHub (per_page=100)
    releasesList = nlohmann::json::array();
    int page = 1;
    for (;;) {
        const auto url = std::format("https://api.github.com/repos/SteamClientHomebrew/Millennium/releases?per_page=100&page={}", page);
        const auto response = Http::Get(url.c_str(), false);

        if (response.empty()) {
            if (page == 1) {
                ShowMessageBox("Whoops!", "Failed to fetch version information from the GitHub API! Make sure you have a valid internet connection.", Error);
                return false;
            }
            break;
        }

        try {
            auto pageJson = nlohmann::json::parse(response);
            if (!pageJson.is_array() || pageJson.empty())
                break;

            // append to releasesList
            for (const auto& r : pageJson)
                releasesList.push_back(r);
        } catch (const nlohmann::json::exception& e) {
            std::cerr << "JSON parse error: " << e.what() << std::endl;
            ShowMessageBox("Whoops!", "Failed to parse version information from the GitHub API! Please wait a moment and try again, you're likely rate limited.", Error);
            return false;
        }

        page += 1;
    }

    // choose default selectedRelease: prefer latest non-prerelease, fall back to first release
    selectedRelease = nlohmann::json();
    for (const auto& release : releasesList) {
        if (release.contains("prerelease") && release["prerelease"].is_boolean() && release["prerelease"].get<bool>() == false) {
            selectedRelease = release;
            break;
        }
    }
    if (selectedRelease.is_null() && !releasesList.empty()) {
        selectedRelease = releasesList.front();
    }

    latestReleaseTag = selectedRelease.contains("tag_name") ? selectedRelease["tag_name"].get<std::string>() : std::string();

    bool hasFoundReleaseInfo = false;
    installSizeStr.clear();
    if (!selectedRelease.is_null()) {
        std::string releaseTag = selectedRelease.contains("tag_name") ? selectedRelease["tag_name"].get<std::string>() : std::string();
        for (const auto& asset : selectedRelease["assets"]) {
            std::string assetName = asset["name"];
#ifdef WIN32
            if (assetName == std::format("millennium-{}-windows-x86_64.zip", releaseTag)) {
                osReleaseInfo = asset;
                hasFoundReleaseInfo = true;
            }
            if (assetName == std::format("millennium-{}-windows-x86_64.installsize", releaseTag)) {
                if (asset.contains("browser_download_url")) {
                    auto sizeResponse = Http::Get(asset["browser_download_url"].get<std::string>().c_str(), false);
                    if (!sizeResponse.empty()) {
                        installSizeStr = sizeResponse;
                        installSizeStr.erase(0, installSizeStr.find_first_not_of(" \t\n\r"));
                        installSizeStr.erase(installSizeStr.find_last_not_of(" \t\n\r") + 1);
                    }
                }
            }
#elif __linux__
            if (assetName == std::format("millennium-{}-linux-x86_64.tar.gz", releaseTag)) {
                osReleaseInfo = asset;
                hasFoundReleaseInfo = true;
            }
            if (assetName == std::format("millennium-{}-linux-x86_64.installsize", releaseTag)) {
                if (asset.contains("browser_download_url")) {
                    auto sizeResponse = Http::Get(asset["browser_download_url"].get<std::string>().c_str(), false);
                    if (!sizeResponse.empty()) {
                        installSizeStr = sizeResponse;
                        installSizeStr.erase(0, installSizeStr.find_first_not_of(" \t\n\r"));
                        installSizeStr.erase(installSizeStr.find_last_not_of(" \t\n\r") + 1);
                    }
                }
            }
#else
            osReleaseInfo = asset;
            hasFoundReleaseInfo = true;
#endif
        }
    }

    return hasFoundReleaseInfo;
}

const void RenderInstallPrompt(std::shared_ptr<RouterNav> router, float xPos)
{
    static std::string steamPath = GetSteamPath();

    ImGuiIO& io = GetIO();
    ImGuiViewport* viewport = GetMainViewport();

    const int XPadding = ScaleX(55);
    const int BottomNavBarHeight = ScaleY(115);

    const int PromptContainerWidth = viewport->Size.x - XPadding * 2;
    const int PromptContainerHeight = viewport->Size.y / 1.5f;

    float startY = viewport->Size.y + BottomNavBarHeight;
    float targetY = viewport->Size.y - BottomNavBarHeight;

    static float currentY = startY;
    static bool animate = true;

    static auto animationStartTime = std::chrono::steady_clock::now();

    SetCursorPos({ xPos + (viewport->Size.x - PromptContainerWidth) / 2, (viewport->Size.y - PromptContainerHeight) / 2 });
    PushStyleColor(ImGuiCol_Border, ImVec4(0.169f, 0.173f, 0.18f, 1.0f));

    BeginChild("##PromptContainer", ImVec2(PromptContainerWidth, PromptContainerHeight), false);
    {
        PushFont(io.Fonts->Fonts[1]);
        Text(std::format("Install Millennium 💫").c_str());
        PopFont();

        Spacing();
        PushStyleColor(ImGuiCol_Text, ImVec4(0.422f, 0.425f, 0.441f, 1.0f));
        // TextWrapped(std::format("Released {} • ", ToTimeAgo(releaseInfo["published_at"].get<std::string>())).c_str());
        // SameLine(0, ScaleX(5));
        // TextColored(ImVec4(0.408f, 0.525f, 0.91f, 1.0f), "view release notes");
        Text("An open source gateway to a better Steam® client experience.");

        if (IsItemHovered()) {
            SetMouseCursor(ImGuiMouseCursor_Hand);
        }
        static bool showModal = false;
        static bool hasSkippedFirstFrame = false;

        if (IsItemClicked()) {
            if (selectedRelease.contains("tag_name")) {
                ShellExecuteA(NULL, "open",
                              std::format("https://github.com/SteamClientHomebrew/Millennium/releases/tag/{}", selectedRelease["tag_name"].get<std::string>()).c_str(), NULL, NULL,
                              SW_SHOWNORMAL);
            }
        }

        Spacing();
        Separator();
        Spacing();
        Spacing();

        Text("Steam Install Path:");
        Spacing();
        Spacing();
        PopStyleColor();

        PushStyleVar(ImGuiStyleVar_FrameRounding, 4);
        PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ScaleX(10), ScaleY(10)));
        PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        PushStyleColor(ImGuiCol_Border, ImVec4(0.18f, 0.184f, 0.192f, 1.0f));
        PushItemWidth(GetContentRegionAvail().x - ScaleX(55));
        InputText("##SteamPath", &steamPath, ImGuiInputTextFlags_ReadOnly);
        PopItemWidth();

        SameLine();
        PushStyleColor(ImGuiCol_Button, ImVec4(0.098f, 0.102f, 0.11f, 1.0f));

        PushFont(io.Fonts->Fonts[1]);
        float buttonCursorPosX = GetCursorPosX();

        if (Button("...", ImVec2(GetContentRegionAvail().x, ScaleY(44)))) {
            auto newSteamPath = SelectNewSteamPath();
            if (newSteamPath.has_value()) {
                steamPath = newSteamPath.value();
            }
        }
        PopFont();

        static bool isOpenFolderHovered = false;
        float currentColor = EaseInOutFloat("##OpenFolderButton", 0.f, 1.f, isOpenFolderHovered, 0.3f);

        /** Check if the animation has started */
        if (currentColor != 0.f) {
            SetMouseCursor(ImGuiMouseCursor_Hand);
            SetNextWindowPos(ImVec2(buttonCursorPosX - ScaleX(183), GetCursorPosY() + ScaleY(120)));

            PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(ScaleX(10), ScaleY(10)));
            PushStyleVar(ImGuiStyleVar_WindowRounding, 6);
            PushStyleVar(ImGuiStyleVar_Alpha, currentColor);
            PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.098f, 0.102f, 0.11f, 1.0f));
            BeginTooltip();
            Text("Select Steam installation path");
            EndTooltip();
            PopStyleVar(3);
            PopStyleColor();
        }

        isOpenFolderHovered = IsItemHovered() || (IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && IsMouseDown(ImGuiMouseButton_Left));

        PopStyleColor(2);
        PopStyleVar(3);

        SetCursorPosY(GetCursorPosY() + ScaleY(100));
        PushStyleColor(ImGuiCol_Text, ImVec4(0.422f, 0.425f, 0.441f, 1.0f));

        std::string currentTag = selectedRelease.contains("tag_name") ? selectedRelease["tag_name"].get<std::string>() : std::string("(none)");
        Text("Installing Millennium version %s", currentTag.c_str());
        SameLine(0, ScaleX(5));

        const char* changeText = "change version ▾";
        ImVec2 changeSize = CalcTextSize(changeText);
        if (changeSize.y < GetTextLineHeight())
            changeSize.y = GetTextLineHeight();

        PushStyleColor(ImGuiCol_Text, ImVec4(0.408f, 0.525f, 0.91f, 1.0f));
        TextUnformatted(changeText);
        if (IsItemHovered())
            SetMouseCursor(ImGuiMouseCursor_Hand);
        if (IsItemClicked())
            OpenPopup("##VersionPopup");

        PopStyleColor();

        ImVec2 popupPos = GetItemRectMin();
        popupPos.y += changeSize.y + ScaleY(10);

        SetNextWindowPos(popupPos);
        PushStyleColor(ImGuiCol_Border, ImVec4(0.18f, 0.184f, 0.192f, 1.0f));
        PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(ScaleX(15), ScaleY(15)));
        PushStyleVar(ImGuiStyleVar_PopupRounding, 6);
        PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.1f, 0.1f, 0.11f, 1.0f));

        SetNextWindowSize(ImVec2(ScaleX(200), ScaleY(200)));

        if (BeginPopup("##VersionPopup")) {
            if (!releasesList.is_null()) {
                for (const auto& release : releasesList) {
                    if (!release.contains("tag_name"))
                        continue;
                    std::string tag = release["tag_name"].get<std::string>();
                    bool is_selected = (selectedRelease.contains("tag_name") && selectedRelease["tag_name"].get<std::string>() == tag);
                    PushStyleVar(ImGuiStyleVar_FrameRounding, 6);

                    std::string strTag = tag;

                    if (latestReleaseTag == tag) {
                        PushStyleColor(ImGuiCol_Text, ImVec4(0.408f, 0.525f, 0.91f, 1.0f));
                        strTag += " (latest)";
                    }

                    if (Selectable(std::format("  {}  ", strTag).c_str(), is_selected)) {
                        UpdateSelectedRelease(tag);
                        CloseCurrentPopup();
                    }

                    if (latestReleaseTag == tag) {
                        PopStyleColor();
                    }

                    PopStyleVar();
                    if (is_selected)
                        SetItemDefaultFocus();
                }
            }
            EndPopup();
        }

        PopStyleVar(2);
        PopStyleColor(3);

        Spacing();
        Spacing();
        Separator();
        Spacing();
        Spacing();

        PushStyleColor(ImGuiCol_Text, ImVec4(0.422f, 0.425f, 0.441f, 1.0f));

        if (installSizeStr.empty()) {
            Text("•   Install size: N/A");
        } else {
            Text("•   Install size: %.2f MB", stof(installSizeStr) / (1024.0f * 1024.0f));
        }

        Text("•   Download size: %.2f MB", osReleaseInfo.contains("size") ? osReleaseInfo["size"].get<float>() / (1024.0f * 1024.0f) : 0.0f);

        PopStyleColor();
    }
    EndChild();
    PopStyleColor();

    RenderBottomNavBar("InstallPrompt", xPos, [router, xPos]
    {
        static bool isButtonHovered = false;
        float currentColor = EaseInOutFloat("##InstallButton", 1.0f, 0.8f, isButtonHovered, 0.3f);

        PushStyleColor(ImGuiCol_Button, ImVec4(currentColor, currentColor, currentColor, 1.0f));
        PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(currentColor, currentColor, currentColor, 1.0f));

        if (Button("Install", ImVec2(xPos + GetContentRegionAvail().x, GetContentRegionAvail().y))) {
            std::thread(StartInstaller, steamPath, std::ref(selectedRelease), std::ref(osReleaseInfo)).detach();
            router->navigateNext();
        }

        if (isButtonHovered) {
            SetMouseCursor(ImGuiMouseCursor_Hand);
        }

        PopStyleColor(2);
        isButtonHovered = IsItemHovered() || (IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && IsMouseDown(ImGuiMouseButton_Left));
    });
}
