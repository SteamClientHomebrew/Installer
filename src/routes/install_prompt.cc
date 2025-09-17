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
#include <windows.h>
#include <shobjidl.h>
#include <commdlg.h>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <http.h>
#include <fmt/format.h>
#include <util.h>
#include <imgui_markdown.h>
#include <mini/ini.h>

using namespace ImGui;

const CheckBoxState* checkForUpdates;
const CheckBoxState* automaticallyInstallUpdates;

static ImGui::MarkdownConfig mdConfig;
nlohmann::json releaseInfo, osReleaseInfo;

const bool FetchVersionInfo()
{
    const auto response = Http::Get("https://api.github.com/repos/SteamClientHomebrew/Millennium/releases", false);

    if (response.empty()) {
        ShowMessageBox("Whoops!", "Failed to fetch version information from the GitHub API! Make sure you have a valid internet connection.", Error);
        return false;
    }

    bool hasFoundReleaseInfo = false;

    try {
        releaseInfo = nlohmann::json::parse(response);
        // Find the latest non-prerelease version
        for (const auto& release : releaseInfo) {
            if (release.contains("prerelease") && release["prerelease"].is_boolean() && release["prerelease"].get<bool>() == false) {
                releaseInfo = release;
                for (const auto& asset : releaseInfo["assets"]) {
                    std::string assetName = asset["name"];
                    std::string releaseTag = releaseInfo["tag_name"];

#ifdef WIN32
                    if (assetName == fmt::format("millennium-{}-windows-x86_64.zip", releaseTag))
#elif __linux__
                    if (assetName == fmt::format("millennium-{}-linux-x86_64.tar.gz", release["tag_name"].get<std::string>()))
#endif
                    {
                        osReleaseInfo = asset;
                        hasFoundReleaseInfo = true;
                        break;
                    }
                }

                break;
            }
        }
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        ShowMessageBox("Whoops!", "Failed to parse version information from the GitHub API! Please wait a moment and try again, you're likely rate limited.", Error);
        return false;
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
        Text("Install Millennium");
        PopFont();

        Spacing();
        PushStyleColor(ImGuiCol_Text, ImVec4(0.422f, 0.425f, 0.441f, 1.0f));
        TextWrapped(fmt::format("Released {} • ", ToTimeAgo(releaseInfo["published_at"].get<std::string>())).c_str());
        SameLine(0, ScaleX(5));
        TextColored(ImVec4(0.408f, 0.525f, 0.91f, 1.0f), "view release notes");

        if (IsItemHovered()) {
            SetMouseCursor(ImGuiMouseCursor_Hand);
        }
        static bool showModal = false;
        static bool hasSkippedFirstFrame = false;

        if (IsItemClicked()) {
            ShellExecuteA(NULL, "open", fmt::format("https://github.com/SteamClientHomebrew/Millennium/releases/tag/{}", releaseInfo["tag_name"].get<std::string>()).c_str(), NULL,
                          NULL, SW_SHOWNORMAL);
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

        SetCursorPosY(GetCursorPosY() + ScaleY(140));
        PushStyleColor(ImGuiCol_Text, ImVec4(0.422f, 0.425f, 0.441f, 1.0f));
        PushStyleColor(ImGuiCol_Text, ImVec4(0.761, 0.569, 0.149, 1.0f));

        Text("Attention");
        PopStyleColor();
        TextWrapped("Your first Steam launch after installing Millennium will take longer than usual while it sets up - don't close Steam during this process.");

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
            std::thread(StartInstaller, steamPath, std::ref(releaseInfo), std::ref(osReleaseInfo)).detach();
            router->navigateNext();
        }

        if (isButtonHovered) {
            SetMouseCursor(ImGuiMouseCursor_Hand);
        }

        PopStyleColor(2);
        isButtonHovered = IsItemHovered() || (IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && IsMouseDown(ImGuiMouseButton_Left));
    });
}
