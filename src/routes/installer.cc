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

#include <router.h>
#include <memory>
#include <imgui.h>
#include <string>
#include <chrono>
#include <animate.h>
#include <texture.h>
#include <imgui_stdlib.h>
#include <imspinner.h>
#include <dpi.h>
#include <thread>
#include <util.h>
#include <nlohmann/json.hpp>
#include <fmt/core.h>
#include <http.h>
#include <task_scheduler.h>
#include <unzip.h>
#include <atomic>
#include <windows.h>
#include <tlhelp32.h>

using namespace ImGui;
using namespace ImSpinner;

static std::string statusText;

float progress = 0.0f;
static float easedProgress = 0.0f;
static float targetProgress = 0.0f;
static const float TRANSITION_DURATION = 0.5f;
static auto transitionStartTime = std::chrono::steady_clock::now();
static float startProgress = 0.0f;

std::unique_ptr<TaskScheduler> scheduler = std::make_unique<TaskScheduler>();

void UpdateProgressEasing()
{
    if (std::abs(targetProgress - progress) > 0.01f) {
        transitionStartTime = std::chrono::steady_clock::now();
        startProgress = easedProgress;
        targetProgress = progress;
    }

    auto now = std::chrono::steady_clock::now();
    float elapsedTime = std::chrono::duration<float>(now - transitionStartTime).count();

    if (elapsedTime < TRANSITION_DURATION) {
        float t = elapsedTime / TRANSITION_DURATION;
        easedProgress = startProgress + (targetProgress - startProgress) * EaseInOut(t);
    } else {
        easedProgress = targetProgress;
    }
}

void DownloadReleaseAssets(std::unique_ptr<double>& progress, const nlohmann::json& releaseInfo, const nlohmann::json& osReleaseInfo)
{
    /** Update the progress text */
    statusText = "Downloading Millennium...";

    const auto fileSize = osReleaseInfo["size"].get<double>();
    const auto downloadUrl = osReleaseInfo["browser_download_url"].get<std::string>();
    /** Download to the temp directory */
    const auto fileName = std::filesystem::temp_directory_path() / osReleaseInfo["name"].get<std::string>();

    Http::downloadFile(downloadUrl, fileName.string(), fileSize, [&progress](double downloaded, double total) { *progress = downloaded / total; }, true);
}

void InstallReleaseAssets(std::unique_ptr<double>& progress, const nlohmann::json& releaseInfo, const nlohmann::json& osReleaseInfo, const std::string& steamPath)
{
    /** Update the progress text */
    statusText = "Installing Millennium...";

    const auto fileName = std::filesystem::temp_directory_path() / osReleaseInfo["name"].get<std::string>();
    double currentFileProgress = 0.0;

    ExtractZippedArchive(fileName.string().c_str(), steamPath.c_str(), progress.get(), &currentFileProgress);
}

std::atomic<bool> hasTaskSchedulerFinished{ false };

void OnFinishInstall()
{
    hasTaskSchedulerFinished.store(true);
}

std::string g_steamPath;

void StartInstaller(std::string steamPath, nlohmann::json& releaseInfo, nlohmann::json& osReleaseInfo)
{
    KillSteamProcess();

    g_steamPath = steamPath;

    progress = 0.0f;
    easedProgress = 0.0f;
    targetProgress = 0.0f;

    scheduler->addTask(std::bind(DownloadReleaseAssets, std::placeholders::_1, releaseInfo, osReleaseInfo));
    scheduler->addTask(std::bind(InstallReleaseAssets, std::placeholders::_1, releaseInfo, osReleaseInfo, steamPath));
    scheduler->run();

    OnFinishInstall();
}

const void RenderInstaller(std::shared_ptr<RouterNav> router, float xPos)
{
    progress = scheduler->getProgress();

    router->setCanGoBack(false);
    UpdateProgressEasing();

    ImGuiIO& io = GetIO();
    ImGuiViewport* viewport = GetMainViewport();
    static const float BottomNavBarHeight = ScaleY(115);

    float startY = viewport->Size.y + BottomNavBarHeight;
    float targetY = viewport->Size.y - BottomNavBarHeight;

    static float currentY = startY;
    static bool shouldAnimate = false;
    static bool shouldRenderCompleteModal = false;

    static auto animationStartTime = std::chrono::steady_clock::now();

    static const int spinnerSize = ScaleX(14);
    static const int progressBarWidth = ScaleX(300);

    if (!shouldRenderCompleteModal) {
        // router->setCanGoBack(true);
        SetCursorPos({ xPos + (viewport->Size.x / 2) - spinnerSize, (viewport->Size.y / 2) - 50 });
        {
            Spinner<SpinnerTypeT::e_st_ang>("SpinnerAngNoBg", Radius{ spinnerSize }, Thickness{ ScaleX(3) }, Color{ ImColor(255, 255, 255, 255) },
                                            BgColor{ ImColor(255, 255, 255, 0) }, Speed{ 6 }, Angle{ IM_PI }, Mode{ 0 });
        }
        SetCursorPos({ xPos + ((viewport->Size.x - progressBarWidth) / 2), viewport->Size.y / 2 + ScaleY(60) });
        {
            ProgressBar(easedProgress, { progressBarWidth, ScaleY(4.0f) }, "##ProgressBar");
        }

        SetCursorPos({ xPos + (viewport->Size.x) / 2 - (CalcTextSize(statusText.c_str()).x / 2), viewport->Size.y / 2 + ScaleY(15) });
        Text(statusText.c_str());
    } else {
        const char* text = "You're all set!  Thanks for using Millennium 💖";
        const char* description = "If you're new here, see further instructions when Steam® starts.";
        const char* subDescription = "View Documentation 🔗";

        PushFont(io.Fonts->Fonts[1]);
        SetCursorPos({ xPos + (viewport->Size.x) / 2 - (CalcTextSize(text).x / 2), viewport->Size.y / 2 - ScaleY(55) });
        Text(text);
        PopFont();

        PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
        SetCursorPos({ xPos + (viewport->Size.x) / 2 - (CalcTextSize(description).x / 2), viewport->Size.y / 2 - ScaleY(15) });
        Text(description);
        PopStyleColor();

        PushStyleColor(ImGuiCol_Text, ImVec4(0.408f, 0.525f, 0.91f, 1.0f));
        SetCursorPos({ xPos + (viewport->Size.x) / 2 - (CalcTextSize(subDescription).x / 2), viewport->Size.y / 2 + ScaleY(20) });
        Text(subDescription);
        PopStyleColor();

        if (IsItemHovered()) {
            SetMouseCursor(ImGuiMouseCursor_Hand);
        }
        if (IsItemClicked()) {
            OpenUrl("https://docs.steambrew.app/users/getting-started");
        }
    }

    if (hasTaskSchedulerFinished.load() && easedProgress == 1.0f) {
        shouldAnimate = true;
        shouldRenderCompleteModal = true;
        animationStartTime = std::chrono::steady_clock::now();

        /** Reset the flag so it doesn't restart the animation each frame */
        hasTaskSchedulerFinished.store(false);
    }

    static const float ANIMATION_DURATION = 0.3f;

    if (shouldAnimate) {
        auto now = std::chrono::steady_clock::now();
        float elapsedTime = std::chrono::duration<float>(now - animationStartTime).count();

        if (elapsedTime < ANIMATION_DURATION) {
            currentY = EaseOutBack(elapsedTime, startY, targetY - startY, ANIMATION_DURATION);
        } else {
            currentY = viewport->Size.y - BottomNavBarHeight;
            shouldAnimate = false;
        }
    }

    if (!shouldRenderCompleteModal) {
        return;
    }

    SetCursorPos({ xPos, currentY });

    PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(ScaleX(30), ScaleY(30)));
    PushStyleColor(ImGuiCol_Border, ImVec4(0.f, 0.f, 0.f, 0.f));
    PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.078f, 0.082f, 0.09f, 1.0f));

    BeginChild("##BottomNavBar", { viewport->Size.x, BottomNavBarHeight - 3 }, true, ImGuiWindowFlags_NoScrollbar);
    {
        SetCursorPos({ ScaleX(45), GetCursorPosY() + ScaleY(12.5) });
        Image((ImTextureID)(intptr_t)infoIconTexture, { ScaleX(25), ScaleY(25) });

        SameLine(0, ScaleX(42));
        const float cursorPosSave = GetCursorPosX();

        SetCursorPosY(GetCursorPosY() - ScaleY(12));
        TextColored(ImVec4(0.322f, 0.325f, 0.341f, 1.0f), "Steam Homebrew & Millennium are not affiliated with");

        SetCursorPos({ cursorPosSave, GetCursorPosY() - ScaleY(20) });
        TextColored(ImVec4(0.322f, 0.325f, 0.341f, 1.0f), "Steam®, Valve, or any of their partners.");

        SameLine(0);
        SetCursorPosY(GetCursorPosY() - ScaleY(25));

        static bool isButtonHovered = false;
        float currentColor = EaseInOutFloat("##NextButton", 1.0f, 0.8f, isButtonHovered, 0.3f);

        PushStyleColor(ImGuiCol_Button, ImVec4(currentColor, currentColor, currentColor, 1.0f));
        PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(currentColor, currentColor, currentColor, 1.0f));
        PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
        PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

        static const int ButtonWidth = ScaleX(150);
        const float buttonPos = GetCursorPosY();

        SetCursorPosX(xPos + GetCursorPosX() + GetContentRegionAvail().x - ButtonWidth);

        if (Button("Finish", { xPos + GetContentRegionAvail().x, GetContentRegionAvail().y })) {
            std::thread(StartSteamFromPath, g_steamPath).detach();
        }

        if (isButtonHovered) {
            SetMouseCursor(ImGuiMouseCursor_Hand);
        }

        PopStyleColor(4);
        isButtonHovered = IsItemHovered() || (IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && IsMouseDown(ImGuiMouseButton_Left));
    }
    EndChild();

    PopStyleVar(2);
    PopStyleColor(2);
}