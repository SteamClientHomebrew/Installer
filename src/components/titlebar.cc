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

#include <exception>
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
#ifndef _WIN32
#include <unistd.h>
#endif

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

        static bool langPopupOpen = false;

        ImVec2 langButtonDimensions = { ceil(ScaleX(50)), ceil(ScaleY(43)) };
        SetCursorPos({ viewport->Size.x - closeButtonDimensions.x - langButtonDimensions.x, 0 });

        PushStyleVar(ImGuiStyleVar_ChildRounding, 0);
        PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.f, 0.f, 0.f, 0.f));
        static bool isLangButtonHovered = false;
        if (isLangButtonHovered)
            PushStyleColor(ImGuiCol_ChildBg, ImVec4(1.f, 1.f, 1.f, 0.06f));

        BeginChild("##LangButton", { langButtonDimensions.x, langButtonDimensions.y }, false, ImGuiWindowFlags_NoScrollbar);
        {
            SetCursorPos({ (langButtonDimensions.x - ScaleX(20)) * 0.5f, ScaleY(12) });
            Image((ImTextureID)(intptr_t)languageIconTexture, { ScaleX(20), ScaleY(20) });
        }
        EndChild();

        ImVec2 langBtnMin = GetItemRectMin();
        ImVec2 langBtnMax = GetItemRectMax();

        if (isLangButtonHovered)
            PopStyleColor();
        PopStyleColor();
        PopStyleVar();

        if (IsItemClicked(ImGuiMouseButton_Left))
            OpenPopup("##LangPopup");

        if (IsItemHovered())
            SetMouseCursor(ImGuiMouseCursor_Hand);

        isLangButtonHovered = IsItemHovered();

        ImGuiIO& io = GetIO();
        ImFont* vietItemFont = (io.Fonts->Fonts.Size > 2) ? io.Fonts->Fonts[2] : nullptr;
        ImFont* dropdownFont = (io.Fonts->Fonts.Size > 3) ? io.Fonts->Fonts[3] : nullptr;

        const auto& langs = Locale::GetAvailableLanguages();
        const std::string& currentLangId = Locale::GetCurrentLanguageId();

        const float popupWidth = ScaleX(400);
        float anim = EaseInOutFloat("##LangPopupAnim", 0.f, 1.f, IsPopupOpen("##LangPopup"), 0.35f);

        float popupY = langBtnMax.y - ScaleY(6) * (1.f - anim);
        SetNextWindowPos({ langBtnMax.x - popupWidth, popupY });
        SetNextWindowSize({ popupWidth, ScaleY(500) });
        SetNextWindowBgAlpha(anim);

        PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.10f, 0.10f, 0.11f, 1.0f));
        PushStyleColor(ImGuiCol_Header, ImVec4(0.20f, 0.21f, 0.22f, 1.0f));
        PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.26f, 0.27f, 0.28f, 1.0f));
        PushStyleColor(ImGuiCol_Border, ImVec4(0.22f, 0.23f, 0.25f, 1.0f));
        PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(ScaleX(6), ScaleY(6)));
        PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ScaleX(8), ScaleY(6)));
        PushStyleVar(ImGuiStyleVar_PopupRounding, ScaleX(4));

        if (dropdownFont)
            PushFont(dropdownFont);

        if (BeginPopup("##LangPopup", ImGuiWindowFlags_NoMove)) {
            if (!::IsWindowFocused())
                CloseCurrentPopup();
            PushStyleVar(ImGuiStyleVar_Alpha, anim);
            for (const auto& lang : langs) {
                bool isSelected = (lang.id == currentLangId);
                bool useVietFont = (lang.id == "vietnamese") && (vietItemFont != nullptr);
                if (useVietFont) {
                    if (dropdownFont)
                        PopFont();
                    PushFont(vietItemFont);
                }
                if (Selectable(lang.displayName.c_str(), isSelected, 0, ImVec2(popupWidth, 0))) {
                    Locale::SetLanguage(lang.id);
                    RequestFontRebuild();
                    CloseCurrentPopup();
                }
                if (isSelected)
                    SetItemDefaultFocus();
                if (useVietFont) {
                    PopFont();
                    if (dropdownFont)
                        PushFont(dropdownFont);
                }
            }
            PopStyleVar();
            EndPopup();
        }

        if (dropdownFont)
            PopFont();

        PopStyleVar(3);
        PopStyleColor(4);

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
#ifdef _WIN32
            std::exit(0);
#else
            _exit(0);
#endif
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

void RenderLanguageSelector(float xPos)
{
}
