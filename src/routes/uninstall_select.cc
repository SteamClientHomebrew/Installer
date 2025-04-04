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
#include <map>
#include <filesystem>
#include <imspinner.h>
#include <util.h>
#include <thread>
#include <fmt/format.h>

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

    ComponentState(bool hovered = false, bool selected = false) : isHovered(hovered), isSelected(selected) {}
};

ComponentProps MakeComponentProps(std::vector<std::filesystem::path> pathList)
{
    uint64_t byteSize = 0;

    for (const auto& path : pathList)
    {
        std::error_code ec;
        byteSize += GetFolderSize(path, ec);
    }

    std::vector<std::string> pathListStr;

    for (const auto& path : pathList)
    {
        pathListStr.push_back(path.string());
    }

    return { byteSize, pathListStr };
}

std::vector<std::pair<std::string, std::tuple<ComponentState, ComponentProps>>> uninstallComponents;

void InitializeUninstaller()
{
    steamPath = GetSteamPath();

    isUninstalling = false;
    uninstallFinished = false;

    uninstallComponents = 
    {
        { "Millennium",                 std::make_tuple(ComponentState({ false, true }), MakeComponentProps({ steamPath / "user32.dll", steamPath / "millennium.dll", steamPath / "python311.dll" }))},
        { "Custom Steam Components",    std::make_tuple(ComponentState({ false, true }), MakeComponentProps({ steamPath / "ext" / "data" / "assets", steamPath / "ext" / "data" / "shims" }))},
        { "Dependencies",               std::make_tuple(ComponentState({ false, true }), MakeComponentProps({ steamPath / "ext" / "data" / "cache" }))},
        { "Themes",                     std::make_tuple(ComponentState({ false, true }), MakeComponentProps({ steamPath / "steamui" / "skins" }))},
        { "Plugins",                    std::make_tuple(ComponentState({ false, true }), MakeComponentProps({ steamPath / "plugins" }))},
    };
}

void StartUninstall()
{
    /** Render all selected components as uninstalling */
    std::for_each(uninstallComponents.begin(), uninstallComponents.end(), [](auto& pair) 
    {
        if (std::get<0>(pair.second).isSelected) std::get<0>(pair.second).uninstallState.state = ComponentState::UninstallState::Uninstalling;
    });
    
    /** Simulate uninstalling process */
    for (auto& componentPair : uninstallComponents)
    {
        auto& [state, props] = componentPair.second;
        if (state.isSelected)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));

            for (const auto& path : props.pathList)
            {
                std::error_code ec;
                std::filesystem::remove_all(path, ec); // Remove the path
                if (ec) 
                {
                    state.uninstallState.state = ComponentState::UninstallState::Failed;
                    state.uninstallState.errorMessage = ec.message();
                }
                else 
                {
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

    for (auto& componentPair : uninstallComponents)
    {
        const auto& [state, props] = componentPair.second;
        totalSize += state.isSelected ? props.byteSize : 0;
    }

    return BytesToReadableFormat(totalSize);
}

const void RenderComponents()
{
    for (auto& componentPair : uninstallComponents)
    {
        auto& component = componentPair.first;
        auto& [state, props] = componentPair.second;

        SetCursorPosY(GetCursorPosY() + ScaleY(5));

        std::string strPathList;

        for (const auto& path : props.pathList)
        {
            strPathList += path + "\n";
        }

        std::string formattedComponent = component + ": " + BytesToReadableFormat(props.byteSize);

        BeginChild(component.c_str(), { ScaleX(35), ScaleY(35) }, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        { 
            /** Not currently uninstalling, the user is selecting components to be uninstalled */
            if (!isUninstalling)
            {
                state.isSelected = RenderCheckBox(state.isSelected, formattedComponent.c_str(), strPathList.c_str(), false, true)->isChecked;
                continue;
            }

            /** Item is excluded from the uninstaller */
            if (!state.isSelected)
            {
                SetCursorPos({ GetCursorPosX() + ScaleX(2), GetCursorPosY() + ScaleY(2) });
                Image((ImTextureID)(intptr_t)excludedIconTexture, { ScaleX(30), ScaleY(30) });
                EndChild();

                if (IsItemHovered())
                {
                    SetMouseCursor(ImGuiMouseCursor_Hand);
                    PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(ScaleX(10), ScaleY(10)));
                    PushStyleVar(ImGuiStyleVar_WindowRounding, 6);
                    PushStyleVar(ImGuiStyleVar_Alpha, 1.f);
                    PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.098f, 0.102f, 0.11f, 1.0f));
                    SetTooltip(fmt::format("\"{}\" was excluded from the removal process.", component).c_str());
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
            switch (state.uninstallState.state)
            {
                case ComponentState::UninstallState::Uninstalling:
                {
                    SetCursorPos({ GetCursorPosX() + spinnerSize / 2, (GetCursorPosY() + spinnerSize / 2) - 5.f });
                    Spinner<SpinnerTypeT::e_st_ang>("SpinnerAngNoBg", Radius{(spinnerSize)}, Thickness{(ScaleX(3))}, Color{ImColor(255, 255, 255, 255)}, BgColor{ImColor(255, 255, 255, 0)}, Speed{6}, Angle{IM_PI}, Mode{0});
                    EndChild();
                    SameLine(0, ScaleX(20));
                    SetCursorPosY(GetCursorPosY() + ScaleY(3));
                    Text(formattedComponent.c_str());
                    break;
                }
                case ComponentState::UninstallState::Success:
                {
                    SetCursorPos({ GetCursorPosX() + ScaleX(2), GetCursorPosY() + ScaleY(2)} );
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

                    if (IsItemHovered())
                    {
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
    
    const int XPadding           = ScaleX(55);
    const int BottomNavBarHeight = ScaleY(115);

    const int PromptContainerWidth  = viewport->Size.x - XPadding * 2;
    const int PromptContainerHeight = viewport->Size.y / 1.5f;

    float startY  = viewport->Size.y + BottomNavBarHeight;
    float targetY = viewport->Size.y - BottomNavBarHeight; 
    
    static float currentY = startY;
    static bool animate   = true;

    static auto animationStartTime = std::chrono::steady_clock::now();

    SetCursorPos({ xPos + (viewport->Size.x - PromptContainerWidth) / 2, (viewport->Size.y - PromptContainerHeight) / 2 });
    PushStyleColor(ImGuiCol_Border, ImVec4(0.169f, 0.173f, 0.18f, 1.0f));

    BeginChild("##PromptContainer", ImVec2(PromptContainerWidth, PromptContainerHeight), false);
    {
        PushFont(io.Fonts->Fonts[1]);
        Text("Uninstall Millennium 😔");
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

    SetCursorPos({ xPos, viewport->Size.y - BottomNavBarHeight + 1 });

    PushStyleVar  (ImGuiStyleVar_WindowPadding, ImVec2(ScaleX(30), ScaleY(30)));
    PushStyleColor(ImGuiCol_Border, ImVec4(0.f, 0.f, 0.f, 0.f));
    PushStyleVar  (ImGuiStyleVar_ChildRounding, 0.0f);
    PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.078f, 0.082f, 0.09f, 1.0f));

    BeginChild("##BottomNavBar", ImVec2(viewport->Size.x, BottomNavBarHeight), true, ImGuiWindowFlags_NoScrollbar);
    {
        SetCursorPos({ ScaleX(45), GetCursorPosY() + ScaleY(12.5) });
        Image((ImTextureID)(intptr_t)infoIconTexture, ImVec2(ScaleX(25), ScaleY(25)));

        SameLine(0, ScaleX(42));
        const float cursorPosSave = GetCursorPosX();

        SetCursorPosY(GetCursorPosY() - ScaleX(12));
        TextColored(ImVec4(0.322f, 0.325f, 0.341f, 1.0f), "Steam Homebrew & Millennium are not affiliated with");

        SetCursorPos({ cursorPosSave, GetCursorPosY() - ScaleY(20) });
        TextColored(ImVec4(0.322f, 0.325f, 0.341f, 1.0f), "Steam®, Valve, or any of their partners.");
        
        SameLine(0);
        SetCursorPosY(GetCursorPosY() - ScaleY(25));

        static bool isButtonHovered = false;
        float currentColor = EaseInOutFloat("##NextButton", 1.0f, 0.8f, isButtonHovered, 0.3f);

        PushStyleColor(ImGuiCol_Button,        ImVec4(currentColor, currentColor, currentColor, 1.0f));
        PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(currentColor, currentColor, currentColor, 1.0f));
        PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
        PushStyleColor(ImGuiCol_Text,          ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

        const int FooterContainerWidth = ScaleX(300);
        const float buttonPos = GetCursorPosY();

        SetCursorPosX(xPos + GetCursorPosX() + GetContentRegionAvail().x - FooterContainerWidth);
        SetCursorPosY(GetCursorPosY() + ScaleY(10));

        Image((ImTextureID)(intptr_t)discordIconTexture, ImVec2(ScaleX(30), ScaleY(30)));
        SameLine(0, ScaleX(25));
        SetCursorPosY(GetCursorPosY() - ScaleY(15));

        Image((ImTextureID)(intptr_t)gtihubIconTexture, ImVec2(ScaleX(30), ScaleY(30)));
        SameLine(0, ScaleX(25));

        SetCursorPosY(buttonPos);

        if (uninstallFinished)
        {
            if (Button("Exit", ImVec2(xPos + GetContentRegionAvail().x, GetContentRegionAvail().y)))
            {
                std::exit(0);
            }
        }
        else
        {
            if (Button("Uninstall", ImVec2(xPos + GetContentRegionAvail().x, GetContentRegionAvail().y)))
            {
                if (isUninstalling)
                {
                    return;
                }

                std::cout << "Uninstalling components..." << std::endl;

                isUninstalling = true;
                std::thread(StartUninstall).detach();
            }
        }

        if (isButtonHovered)
        {
            SetMouseCursor(ImGuiMouseCursor_Hand);
        }

        PopStyleColor(4);
        isButtonHovered = IsItemHovered() || (IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && IsMouseDown(ImGuiMouseButton_Left));
    }
    EndChild();

    PopStyleVar(2);
    PopStyleColor(2);
}