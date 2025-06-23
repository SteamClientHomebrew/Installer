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
#include <memory>
#include <router.h>
#include <imgui_internal.h>
#include <iostream>
#include <dpi.h>
#include <unordered_map>
#include <components.h>
#include <thread>
#include <atomic>
#include <imspinner.h>

using namespace ImGui;
using namespace ImSpinner;

enum OptionType
{
    UNSET,
    INSTALL,
    REMOVE
};

struct OptionProps
{
    std::string title; 
    std::string description;
    OptionType type;
};

struct OptionState 
{
    bool isHovered;
    bool isSelected;
    float initPos;
};

std::unordered_map<std::string, OptionState> optionStates;
static OptionType currentOption = UNSET;

const void RenderOption(OptionProps props, int ContainerWidth, int ContainerHeight, int ContainerSpacing)
{   
    ImGuiIO& io = GetIO();
    const float DEFAULT_BORDER_COL = GetStyleColorVec4(ImGuiCol_Border).x;

    auto& state   = optionStates[props.title];
    state.initPos = GetCursorPosY();

    static const float hoverColor    = 0.22f;
    static const float selectedColor = 1.0f;

    float currentColor        = SmoothFloat(props.title, state.initPos, state.isHovered, 5, 0.15f, std::make_tuple(DEFAULT_BORDER_COL,hoverColor));
    float currentClickedColor = EaseInOutFloat(props.title, hoverColor, selectedColor, state.isSelected, 0.3f);

    if (state.isHovered && currentClickedColor == hoverColor)
    {
        PushStyleColor(ImGuiCol_Border, ImVec4(currentColor, currentColor, currentColor, 1.0f));
    }
    else if (currentClickedColor != hoverColor)
    {
        PushStyleColor(ImGuiCol_Border, ImVec4(currentClickedColor, currentClickedColor, currentClickedColor, 1.f));
    }

    PushStyleVar(ImGuiStyleVar_ChildRounding, ScaleX(12.0f));
    PushStyleVar(ImGuiStyleVar_ChildRounding, ScaleX(12.0f));
    BeginChild(("##" + props.title + "Container").c_str(), ImVec2(ContainerWidth, ContainerHeight), ImGuiChildFlags_Border, ImGuiWindowFlags_NoScrollbar);
    {
        PushFont(io.Fonts->Fonts[1]);
        Text("%s", props.title.c_str());
        PopFont();

        Spacing();
        PushStyleColor(ImGuiCol_Text, ImVec4(0.422f, 0.425f, 0.441f, 1.0f));
        TextWrapped("%s", props.description.c_str());
        PopStyleColor();
    }
    EndChild();
    PopStyleVar(2);

    if (state.isHovered || currentClickedColor != hoverColor)
    {
        PopStyleColor();
    }

    if (IsItemClicked())
    {
        state.isSelected = !state.isSelected;

        if (state.isSelected)
        {
            currentOption = props.type;
        }
        else
        {
            currentOption = UNSET;
        }

        /** Unselect other options */
        if (state.isSelected)
        {
            for (auto& [key, value] : optionStates)
            {
                if (key != props.title)
                {
                    value.isSelected = false;
                }
            }
        }
    }

    state.isHovered = IsItemHovered() || (IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && IsMouseDown(ImGuiMouseButton_Left));
    SameLine(0, ContainerSpacing);
    SetCursorPosY(state.initPos);
}

const float PollAnimationFrames(float navbarHeight, float windowHeight)
{
    static const float ANIMATION_DURATION = 0.3f;
    static bool shouldAnimate = true;

    float startY =  windowHeight + navbarHeight;
    float targetY = windowHeight - navbarHeight; 
    
    static float currentY = startY;
    static auto animationStartTime = std::chrono::steady_clock::now();

    if (shouldAnimate) 
    {
        auto steadyClockNow = std::chrono::steady_clock::now();
        float elapsedTime = std::chrono::duration<float>(steadyClockNow - animationStartTime).count();

        if (elapsedTime < ANIMATION_DURATION) 
        {
            currentY = EaseOutBack(elapsedTime, startY, targetY - startY, ANIMATION_DURATION);
        } 
        else 
        {
            currentY = windowHeight - navbarHeight;
            shouldAnimate = false;
        }
    }
    else 
    {
        currentY = windowHeight - navbarHeight;
    }

    return currentY;
}

static std::vector<Component> InstallComponents 
{
    RenderHome,
    RenderInstallPrompt,
    RenderInstaller
};

static std::vector<Component> UninstallComponents 
{
    RenderHome,
    RenderUninstallSelect
};

const void RenderHome(std::shared_ptr<RouterNav> router, float xPos)
{
    ImGuiViewport* viewport = GetMainViewport();

    const int BottomNavBarHeight = ScaleY(115);
    const int ContainerHeight = ScaleY(150); // ImGui Scaling manages DPI here.  
    const int ContainerSpacing = ScaleX(30);
    const int ContainerWidth = (viewport->Size.x - ScaleX(100)) / 2;

    PushStyleColor(ImGuiCol_Border, ImVec4(0.169f, 0.173f, 0.18f, 1.0f));
    
    SetCursorPos
    ({
        xPos + (viewport->Size.x - ((ContainerWidth * 2) + ContainerSpacing)) / 2,
        ((viewport->Size.y - BottomNavBarHeight) / 2.0f) - ContainerHeight / 2
    });

    RenderOption({ "Install", "Integrate Millennium into your Steam® Client.", INSTALL }, ContainerWidth, ContainerHeight, ContainerSpacing);
    RenderOption({ "Remove" , "Selectively uninstall portions of Millennium.", REMOVE  }, ContainerWidth, ContainerHeight, ContainerSpacing);

    PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.078f, 0.082f, 0.09f, 1.0f));

    /** Animate the footer bar into view */
    SetCursorPos({ xPos, PollAnimationFrames(BottomNavBarHeight, viewport->Size.y) });

    PushStyleVar  (ImGuiStyleVar_WindowPadding, ImVec2(ScaleX(30), ScaleY(30)));
    PushStyleColor(ImGuiCol_Border, ImVec4(0.f, 0.f, 0.f, 0.f));
    PushStyleVar  (ImGuiStyleVar_ChildRounding, 0.0f);

    static bool isButtonHovered = false;
    float currentColor = EaseInOutFloat("##NextButton", 1.0f, 0.8f, isButtonHovered, 0.3f);

    PushStyleColor(ImGuiCol_Button,        ImVec4(currentColor, currentColor, currentColor, 1.0f));
    PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(currentColor, currentColor, currentColor, 1.0f));
    PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
    PushStyleColor(ImGuiCol_Text,          ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

    RenderBottomNavBar("HomePanel", xPos, [xPos, router, viewport] 
    {
        static std::atomic<bool> isLoading { false };
        bool hasLoadedPushed = false;

        if (currentOption == UNSET || isLoading.load(std::memory_order_relaxed))
        {
            PushStyleColor(ImGuiCol_Button,        ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

            hasLoadedPushed = true;
        }

        if (isLoading.load(std::memory_order_relaxed))
        {
            PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            PushStyleVar(ImGuiStyleVar_ChildRounding, ScaleX(10.0f));

            const float childWidth  = GetContentRegionAvail().x;
            const float childHeight = GetContentRegionAvail().y;

            BeginChild("##ButtonIsBusy", { xPos + childWidth, childHeight }, true, ImGuiWindowFlags_NoScrollbar);
            {
                const float spinnerSize = ScaleX(12.f);
                SetCursorPos({ childWidth / 2 - spinnerSize, spinnerSize });

                Spinner<SpinnerTypeT::e_st_ang>("SpinnerAngNoBg", Radius{spinnerSize}, Thickness{ScaleX(2)}, Color{ImColor(0, 0, 0, 255)}, BgColor{ImColor(255, 255, 255, 0)}, Speed{6}, Angle{IM_PI}, Mode{0});
            }
            EndChild();
            PopStyleColor();
            PopStyleVar();
        }
        else if (Button("Next", { xPos + GetContentRegionAvail().x, GetContentRegionAvail().y }))
        {
            switch (currentOption)
            {
                case INSTALL:
                {
                    isLoading.store(true, std::memory_order_relaxed);
                    router->setComponents(InstallComponents);

                    const auto StartInstall = [router]() 
                    {
                        if (FetchVersionInfo())
                        {
                            router->navigateNext();
                        }
                        isLoading.store(false, std::memory_order_relaxed);
                    };

                    std::thread(StartInstall).detach();
                    break;
                }
                case REMOVE:
                {
                    isLoading.store(true, std::memory_order_relaxed);
                    router->setComponents(UninstallComponents);

                    const auto StartUninstall = [router]() 
                    {
                        InitializeUninstaller();
                        router->navigateNext();
                        isLoading.store(false, std::memory_order_relaxed);
                    };

                    std::thread(StartUninstall).detach();
                    break;
                }
                default:
                {
                    break;
                }
            }
        }
    
        if (currentOption == UNSET || hasLoadedPushed)
        {
            PopStyleColor(3);
        }

        const bool buttonHovered = IsItemHovered() || (IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && IsMouseDown(ImGuiMouseButton_Left));

        if (buttonHovered)
        {
            if (currentOption == UNSET || isLoading.load(std::memory_order_relaxed))
            {
                SetMouseCursor(ImGuiMouseCursor_NotAllowed);
            }
            else 
            {
                SetMouseCursor(ImGuiMouseCursor_Hand);  
            }
        }

        const char* toolTipText  = "Select an option above to continue.";
        const float tooltipWidth = CalcTextSize(toolTipText).x + ScaleX(20);

        float toolTipOpacity = EaseInOutFloat("##SelectAnOptionTooltip", 0.f, 1.f, buttonHovered && currentOption == UNSET, 0.4f);

        if (toolTipOpacity != 0.f)
        {
            SetNextWindowPos({ (viewport->Size.x - tooltipWidth) / 2, viewport->Size.y - ScaleY(220) });

            PushStyleColor( ImGuiCol_Border,  ImVec4(0.18f,  0.184f, 0.192f, 1.0f));
            PushStyleColor( ImGuiCol_Text,    ImVec4(0.422f, 0.425f, 0.441f, 1.0f));
            PushStyleColor( ImGuiCol_PopupBg, ImVec4(0.098f, 0.102f, 0.11f,  1.0f));

            PushStyleVar  ( ImGuiStyleVar_WindowPadding, ImVec2(ScaleX(10), ScaleY(10)));
            PushStyleVar  ( ImGuiStyleVar_WindowRounding, 6);
            PushStyleVar  ( ImGuiStyleVar_Alpha, toolTipOpacity);

            BeginTooltip();
            Text(toolTipText);
            EndTooltip();

            PopStyleVar(3);
            PopStyleColor(3);
        }

        PopStyleColor(3);

        if (currentOption != UNSET)
        {
            isButtonHovered = buttonHovered;
        }
    });

    PopStyleVar(2);
    PopStyleColor(4);
}