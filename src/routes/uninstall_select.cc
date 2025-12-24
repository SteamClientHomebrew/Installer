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
#include <texture.hh>
#include <imgui_stdlib.h>
#include <imgui_internal.h>
#include <memory>
#include <router.h>
#include <iostream>
#include <dpi.h>
#include <unordered_map>
#include <components.h>
#include <map>
#include <filesystem>
#include <imspinner.h>
#include <util.h>
#include <thread>

using namespace ImGui;
using namespace ImSpinner;

static std::filesystem::path steamPath;

struct ComponentProps
{
    float byteSize;
    std::vector<std::string> pathList;
};

bool isUninstalling = false;
bool uninstallFinished = false;

struct ComponentState
{
    bool isHovered;
    bool isSelected;

    struct UninstallState
    {
        enum State
        {
            Uninstalling,
            Success,
            Failed
        };

        std::optional<std::string> errorMessage;
        State state;
    };

    UninstallState uninstallState;

    ComponentState(bool hovered = false, bool selected = false) : isHovered(hovered), isSelected(selected)
    {
    }
};

ComponentProps MakeComponentProps(std::vector<std::filesystem::path> pathList)
{
    uint64_t byteSize = 0;

    for (const auto& path : pathList) {
        std::error_code ec;
        byteSize += GetFolderSize(path, ec);
    }

    std::vector<std::string> pathListStr;

    for (const auto& path : pathList) {
        if (!std::filesystem::exists(path)) {
            continue;
        }
        pathListStr.push_back(path.string());
    }

    return { static_cast<float>(byteSize), pathListStr };
}

std::vector<std::pair<std::string, std::tuple<ComponentState, ComponentProps>>> uninstallComponents;

// clang-format off
void InitializeUninstaller()
{
    steamPath = GetSteamPath();

    isUninstalling = false;
    uninstallFinished = false;

    uninstallComponents = {
        { "Millennium", std::make_tuple(ComponentState({ false, true }), MakeComponentProps({ 
            steamPath / "user32.dll", 
            steamPath / "version.dll", 
            steamPath / "ext" / "compat32" / "millennium_x86.dll", 
            steamPath / "ext" / "compat32" / "python311.dll", 
            steamPath / "ext" / "compat64" / "millennium_x64.dll", 
            steamPath / "ext" / "compat64" / "python311.dll", 
            steamPath / "millennium.hhx64.dll", 
            steamPath / "millennium.dll", 
            steamPath / "python311.dll" 
        })) },
        { "Custom Steam Components", std::make_tuple(ComponentState({ false, true }), MakeComponentProps({ 
            steamPath / "ext" / "data" / "assets", 
            steamPath / "ext" / "data" / "shims" 
        })) },
        { "Dependencies", std::make_tuple(ComponentState({ false, true }), MakeComponentProps({ steamPath / "ext" / "data" / "cache", steamPath / "ext" / "data" / "pyx64" })) },
        { "Themes",       std::make_tuple(ComponentState({ false, true }), MakeComponentProps({ steamPath / "steamui" / "skins" }))                                            },
        { "Plugins",      std::make_tuple(ComponentState({ false, true }), MakeComponentProps({ steamPath / "plugins" }))                                                      },
    };
}
// clang-format on

void StartUninstall()
{
    /** Kill Steam before uninstalling */
    KillSteamProcess();

    /** Render all selected components as uninstalling */
    std::for_each(uninstallComponents.begin(), uninstallComponents.end(), [](auto& pair)
    {
        if (std::get<0>(pair.second).isSelected)
            std::get<0>(pair.second).uninstallState.state = ComponentState::UninstallState::Uninstalling;
    });

    /** Simulate uninstalling process */
    for (auto& componentPair : uninstallComponents) {
        auto& [state, props] = componentPair.second;
        if (state.isSelected) {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));

            /** If the path list is empty, the component is already uninstalled */
            if (props.pathList.empty()) {
                state.uninstallState.state = ComponentState::UninstallState::Success;
                continue;
            }

            for (const auto& path : props.pathList) {
                std::error_code ec;
                std::filesystem::remove_all(path, ec); // Remove the path
                if (ec) {
                    state.uninstallState.state = ComponentState::UninstallState::Failed;
                    state.uninstallState.errorMessage = ec.message();
                } else {
                    state.uninstallState.state = ComponentState::UninstallState::Success;
                }
            }
        }
    }

    uninstallFinished = true;
}

std::string GetReclaimedSpace()
{
    uint64_t totalSize = 0;

    for (auto& componentPair : uninstallComponents) {
        const auto& [state, props] = componentPair.second;
        totalSize += state.isSelected ? props.byteSize : 0;
    }

    return BytesToReadableFormat(totalSize);
}

const void RenderComponents()
{
    for (auto& componentPair : uninstallComponents) {
        auto& component = componentPair.first;
        auto& [state, props] = componentPair.second;

        SetCursorPosY(GetCursorPosY() + ScaleY(5));

        std::string strPathList;

        for (const auto& path : props.pathList) {
            strPathList += path + "\n";
        }

        std::string formattedComponent = component + ": " + BytesToReadableFormat(props.byteSize);

        BeginChild(component.c_str(), { ScaleX(35), ScaleY(35) }, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        {
            /** Not currently uninstalling, the user is selecting components to be uninstalled */
            if (!isUninstalling) {
                state.isSelected = RenderCheckBox(state.isSelected, formattedComponent, strPathList, false, true)->isChecked;
                continue;
            }

            /** Item is excluded from the uninstaller */
            if (!state.isSelected) {
                SetCursorPos({ GetCursorPosX() + ScaleX(2), GetCursorPosY() + ScaleY(2) });
                Image((ImTextureID)(intptr_t)excludedIconTexture, { ScaleX(30), ScaleY(30) });
                EndChild();

                if (IsItemHovered()) {
                    SetMouseCursor(ImGuiMouseCursor_Hand);
                    PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(ScaleX(10), ScaleY(10)));
                    PushStyleVar(ImGuiStyleVar_WindowRounding, 6);
                    PushStyleVar(ImGuiStyleVar_Alpha, 1.f);
                    PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.098f, 0.102f, 0.11f, 1.0f));
                    SetTooltip(std::format("\"{}\" was excluded from the removal process.", component).c_str());
                    PopStyleVar(3);
                    PopStyleColor();
                }

                SameLine(0, ScaleX(20));
                SetCursorPosY(GetCursorPosY() + ScaleY(3));
                Text(formattedComponent.c_str());

                continue;
            }
            const float spinnerSize = ScaleX(10);

            /** Render the state of the uninstalling components */
            switch (state.uninstallState.state) {
                case ComponentState::UninstallState::Uninstalling:
                {
                    SetCursorPos({ GetCursorPosX() + spinnerSize / 2, (GetCursorPosY() + spinnerSize / 2) - 5.f });
                    Spinner<SpinnerTypeT::e_st_ang>("SpinnerAngNoBg", Radius{ (spinnerSize) }, Thickness{ (ScaleX(3)) }, Color{ ImColor(255, 255, 255, 255) },
                                                    BgColor{ ImColor(255, 255, 255, 0) }, Speed{ 6 }, Angle{ IM_PI }, Mode{ 0 });
                    EndChild();
                    SameLine(0, ScaleX(20));
                    SetCursorPosY(GetCursorPosY() + ScaleY(3));
                    Text(formattedComponent.c_str());
                    break;
                }
                case ComponentState::UninstallState::Success:
                {
                    SetCursorPos({ GetCursorPosX() + ScaleX(2), GetCursorPosY() + ScaleY(2) });
                    Image((ImTextureID)(intptr_t)successIconTexture, { ScaleX(30), ScaleY(30) });
                    EndChild();

                    SameLine(0, ScaleX(20));
                    SetCursorPosY(GetCursorPosY() + ScaleY(3));

                    Text((component + ": 0.00 B").c_str());
                    break;
                }
                case ComponentState::UninstallState::Failed:
                {
                    SetCursorPos({ GetCursorPosX() + ScaleX(2), GetCursorPosY() + ScaleY(2) });
                    Image((ImTextureID)(intptr_t)errorIconTexture, ImVec2(ScaleX(30), ScaleY(30)));
                    EndChild();

                    if (IsItemHovered()) {
                        SetMouseCursor(ImGuiMouseCursor_Hand);
                        PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(ScaleX(10), ScaleY(10)));
                        PushStyleVar(ImGuiStyleVar_WindowRounding, 6);
                        PushStyleVar(ImGuiStyleVar_Alpha, 1.f);
                        PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.098f, 0.102f, 0.11f, 1.0f));
                        SetTooltip("Failed to uninstall \"%s\".\n\nError: %s", component.c_str(), state.uninstallState.errorMessage.value_or("Unknown error").c_str());
                        PopStyleVar(3);
                        PopStyleColor();
                    }

                    SameLine(0, ScaleX(20));
                    SetCursorPosY(GetCursorPosY() + ScaleY(3));
                    Text(formattedComponent.c_str());

                    break;
                }
                default:
                {
                    EndChild();
                    break;
                }
            }
        }
    }
}

const void RenderUninstallSelect(std::shared_ptr<RouterNav> router, float xPos)
{
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
        Text("Uninstall Millennium 🙁");
        PopFont();

        SetCursorPosY(GetCursorPosY() + ScaleY(5));
        PushStyleColor(ImGuiCol_Text, ImVec4(0.422f, 0.425f, 0.441f, 1.0f));
        TextWrapped("Select the components you would like to remove.");

        SetCursorPosY(GetCursorPosY() + ScaleY(30));
        RenderComponents();
        SetCursorPosY(GetCursorPosY() + ScaleY(40));

        Separator();
        SetCursorPosY(GetCursorPosY() + ScaleY(5));

        Text("Uninstalling from: %s", steamPath.string().c_str());
        Text("Reclaimed Disk Space: %s", GetReclaimedSpace().c_str());

        PopStyleColor();
    }
    EndChild();
    PopStyleColor();

    RenderBottomNavBar("InstallPrompt", xPos, [router, xPos]
    {
        static bool isButtonHovered = false;

        if (uninstallFinished) {
            float currentColor = EaseInOutFloat("##NextButton", 1.0f, 0.8f, isButtonHovered, 0.3f);

            PushStyleColor(ImGuiCol_Button, ImVec4(currentColor, currentColor, currentColor, 1.0f));
            PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(currentColor, currentColor, currentColor, 1.0f));
            PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));

            if (Button("Exit", ImVec2(xPos + GetContentRegionAvail().x, GetContentRegionAvail().y))) {
                std::exit(0);
            }

            if (isButtonHovered) {
                SetMouseCursor(ImGuiMouseCursor_Hand);
            }

            PopStyleColor(3);
            isButtonHovered = IsItemHovered() || (IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && IsMouseDown(ImGuiMouseButton_Left));
        } else if (isUninstalling) {
            PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            PushStyleVar(ImGuiStyleVar_ChildRounding, ScaleX(10.0f));

            const float childWidth = GetContentRegionAvail().x;
            const float childHeight = GetContentRegionAvail().y;

            BeginChild("##ButtonIsBusy", { xPos + childWidth, childHeight }, true, ImGuiWindowFlags_NoScrollbar);
            {
                const float spinnerSize = ScaleX(12.f);
                SetCursorPos({ childWidth / 2 - spinnerSize, spinnerSize });

                Spinner<SpinnerTypeT::e_st_ang>("SpinnerAngNoBg", Radius{ spinnerSize }, Thickness{ ScaleX(2) }, Color{ ImColor(0, 0, 0, 255) },
                                                BgColor{ ImColor(255, 255, 255, 0) }, Speed{ 6 }, Angle{ IM_PI }, Mode{ 0 });
            }
            EndChild();
            PopStyleColor();
            PopStyleVar();

            if (IsItemHovered()) {
                SetMouseCursor(ImGuiMouseCursor_NotAllowed);
            }
        } else {
            float currentColor = EaseInOutFloat("##NextButton", 1.0f, 0.8f, isButtonHovered, 0.3f);

            PushStyleColor(ImGuiCol_Button, ImVec4(currentColor, currentColor, currentColor, 1.0f));
            PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(currentColor, currentColor, currentColor, 1.0f));
            PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));

            if (Button("Uninstall", ImVec2(xPos + GetContentRegionAvail().x, GetContentRegionAvail().y))) {
                std::cout << "Uninstalling components..." << std::endl;

                isUninstalling = true;
                std::thread(StartUninstall).detach();
            }

            if (isButtonHovered) {
                SetMouseCursor(ImGuiMouseCursor_Hand);
            }

            PopStyleColor(3);
            isButtonHovered = IsItemHovered() || (IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && IsMouseDown(ImGuiMouseButton_Left));
        }
    });
}