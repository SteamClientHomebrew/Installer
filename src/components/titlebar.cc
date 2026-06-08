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
#include <i18n.h>
#include <math.h>
#include <worker.h>
#include <renderer.h>
#include <cstdlib>
#include <format>

using namespace ImGui;

/**
 * Render the title bar component.
 * @return Whether the title bar component is hovered.
 */
bool RenderTitleBarComponent(std::shared_ptr<RouterNav> router)
{
    const std::string strTitleText = Locale::Get("titlebarTitle");

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
        Text("%s", strTitleText.c_str());
        SameLine();

        ImVec2 closeButtonDimensions = { ceil(ScaleX(70)), ceil(ScaleY(43)) };

        static bool isCloseButtonHovered = false;

        if (isCloseButtonHovered) {
            PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.769f, 0.169f, 0.11f, 1.0f));
        }

        SetCursorPos({ viewport->Size.x - closeButtonDimensions.x, 0 });

        PushStyleVar(ImGuiStyleVar_ChildRounding, 0);
        BeginChild("##CloseButton", { closeButtonDimensions.x, closeButtonDimensions.y }, true, ImGuiWindowFlags_NoScrollbar);
        {
            SetCursorPos({ ScaleX(25), ScaleY(12) });
            Image((ImTextureID)(intptr_t)closeButtonTexture, { closeButtonDim, closeButtonDim });
        }
        PopStyleVar();
        EndChild();

        if (IsItemClicked(ImGuiMouseButton_Left) && !IsWorkerBusy()) {
            JoinWorker();
            std::exit(0);
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

/**
 * Render the language selector dropdown in the main window scope (below the
 * WndProc drag zone, top-right of the content area).  Must be called from
 * the root Begin/End block, NOT from inside a BeginChild.
 */
void RenderLanguageSelector(float xPos)
{
    ImGuiViewport* viewport = GetMainViewport();

    const auto&        langs         = Locale::GetAvailableLanguages();
    const std::string& currentLangId = Locale::GetCurrentLanguageId();

    std::string previewValue;
    for (const auto& lang : langs) {
        if (lang.id == currentLangId) { previewValue = lang.displayName; break; }
    }
    if (previewValue.empty()) previewValue = currentLangId;

    const float langSelectorWidth = ScaleX(170);
    // ScaleY(105): safely below the WndProc drag zone (ScaleY(100)), top-right of content
    // Right edge aligned with the right edge of the option cards (ScaleX(35))
    SetCursorPos({ xPos + viewport->Size.x - langSelectorWidth - ScaleX(35), ScaleY(105) });

    PushStyleColor(ImGuiCol_FrameBg,        ImVec4(0.13f, 0.14f, 0.15f, 1.0f));
    PushStyleColor(ImGuiCol_FrameBgHovered,  ImVec4(0.19f, 0.20f, 0.21f, 1.0f));
    PushStyleColor(ImGuiCol_FrameBgActive,   ImVec4(0.16f, 0.17f, 0.18f, 1.0f));
    PushStyleColor(ImGuiCol_PopupBg,         ImVec4(0.10f, 0.10f, 0.11f, 1.0f));
    PushStyleColor(ImGuiCol_Header,          ImVec4(0.20f, 0.21f, 0.22f, 1.0f));
    PushStyleColor(ImGuiCol_HeaderHovered,   ImVec4(0.26f, 0.27f, 0.28f, 1.0f));
    PushStyleColor(ImGuiCol_Border,          ImVec4(0.22f, 0.23f, 0.25f, 1.0f));
    PushStyleVar(ImGuiStyleVar_FrameRounding, ScaleX(4));
    PushStyleVar(ImGuiStyleVar_FramePadding,  ImVec2(ScaleX(8), ScaleY(7)));
    PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(ScaleX(8), 0));

    // Fonts[2] = VietName_Standalone: pushed for the Vietnamese item only.
    // Fonts[3] = Geist+CJK+VietPreview: present only when Vietnamese is active;
    //            pushed around the entire combo so it looks the same as English mode.
    ImGuiIO& io = GetIO();
    ImFont* vietItemFont = (io.Fonts->Fonts.Size > 2) ? io.Fonts->Fonts[2] : nullptr;
    ImFont* dropdownFont = (io.Fonts->Fonts.Size > 3) ? io.Fonts->Fonts[3] : nullptr;

    if (dropdownFont) PushFont(dropdownFont);

    SetNextItemWidth(langSelectorWidth);
    if (BeginCombo("##LangSelector", previewValue.c_str())) {
        for (const auto& lang : langs) {
            bool isSelected = (lang.id == currentLangId);
            bool useVietFont = (lang.id == "vietnamese") && (vietItemFont != nullptr);
            if (useVietFont) {
                if (dropdownFont) PopFont();
                PushFont(vietItemFont);
            }
            if (Selectable(lang.displayName.c_str(), isSelected)) {
                Locale::SetLanguage(lang.id);
                RequestFontRebuild();
            }
            if (isSelected)
                SetItemDefaultFocus();
            if (useVietFont) {
                PopFont();
                if (dropdownFont) PushFont(dropdownFont);
            }
        }
        EndCombo();
    }

    if (dropdownFont) PopFont();

    if (IsItemHovered())
        SetMouseCursor(ImGuiMouseCursor_Hand);

    PopStyleVar(3);
    PopStyleColor(7);
}
