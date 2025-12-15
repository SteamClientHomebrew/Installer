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
#include <texture.hh>
#include <dpi.h>
#include <components.h>
#include <animate.h>
#include <iostream>
#include <math.h>

using namespace ImGui;

/**
 * Render the title bar component.
 * @return Whether the title bar component is hovered.
 */
bool RenderTitleBarComponent(std::shared_ptr<RouterNav> router)
{
    ImGuiIO& io = GetIO();
    const std::string strTitleText = std::format("Steam Homebrew", io.Framerate);

    ImGuiViewport* viewport = GetMainViewport();
    PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(ScaleX(15), ScaleY(15)));

    BeginChild("##TitleBar", ImVec2(viewport->Size.x, ScaleY(75)), false, ImGuiWindowFlags_NoScrollbar);
    {
        float iconDimension = ScaleX(20);
        float closeButtonDim = ScaleY(20);
        float titlePadding = ScaleX(20);
        float backButtonPos = ScaleX(EaseInOutFloat("##TitleBarBackButton", 0.f, 45, !router->canGoBack(), 0.3f));

        SetCursorPos({ ScaleX(5) - backButtonPos, ScaleX(5) });

        ImVec2 backButtonDim = { ScaleX(45), ScaleY(43) };

        PushStyleVar(ImGuiStyleVar_ChildRounding, ScaleX(6));
        BeginChild("##BackButtonChild", ImVec2(backButtonDim.x, backButtonDim.y), true, ImGuiWindowFlags_NoScrollbar);
        {
            SetCursorPos({ ScaleX(13), ScaleY(11) });
            Image((ImTextureID)(intptr_t)backBtnTexture, { iconDimension, iconDimension });
        }
        PopStyleVar();
        EndChild();
        SameLine();

        SetCursorPosY(ScaleY(16));

        if (IsItemClicked(ImGuiMouseButton_Left)) {
            router->navigateBack();
        }

        if (IsItemHovered()) {
            SetMouseCursor(ImGuiMouseCursor_Hand);
        }

        Image((ImTextureID)(intptr_t)logoTexture, ImVec2(iconDimension, iconDimension));

        SameLine(0, titlePadding);
        SetCursorPosY(GetCursorPosY() + ScaleY(10));
        Text(strTitleText.c_str());
        SameLine();

        static bool isCloseButtonHovered = false;

        if (isCloseButtonHovered) {
            PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.769f, 0.169f, 0.11f, 1.0f));
        }

        ImVec2 closeButtonDimensions = { ceil(ScaleX(70)), ceil(ScaleY(43)) };
        SetCursorPos({ viewport->Size.x - closeButtonDimensions.x, 0 });

        PushStyleVar(ImGuiStyleVar_ChildRounding, 0);
        BeginChild("##CloseButton", { closeButtonDimensions.x, closeButtonDimensions.y }, true, ImGuiWindowFlags_NoScrollbar);
        {
            SetCursorPos({ ScaleX(25), ScaleY(12) });
            Image((ImTextureID)(intptr_t)closeButtonTexture, { closeButtonDim, closeButtonDim });
        }
        PopStyleVar();
        EndChild();

        if (IsItemClicked(ImGuiMouseButton_Left)) {
            ExitProcess(0);
        }

        if (isCloseButtonHovered) {
            PopStyleColor();
        }

        isCloseButtonHovered = IsItemHovered();
    }
    EndChild();
    PopStyleVar();

    return IsItemHovered() || (IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && IsMouseDown(ImGuiMouseButton_Left));
}