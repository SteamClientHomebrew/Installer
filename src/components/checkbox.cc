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

#include <components.h>
#include <dpi.h>
#include <router.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <iostream>
#include <unordered_map>
#include <animate.h>

using namespace ImGui;

const CheckBoxState* RenderCheckBox(bool checked, std::string description, std::string tooltipText, bool disabled, bool endChild)
{
    static std::unordered_map<std::string, CheckBoxState> checkBoxStates;
    auto& state = checkBoxStates.try_emplace(description, checked).first->second;

    PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
    PushStyleColor(ImGuiCol_Border, ImVec4(0.18f, 0.184f, 0.192f, 1.0f));
    PushStyleColor(ImGuiCol_CheckMark, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.098f, 0.102f, 0.11f, 1.0f));

    BeginDisabled(disabled);

    if (disabled) {
        state.isChecked = false;
    }

    if (endChild) {
        Checkbox("##Checkbox", &state.isChecked);
        EndDisabled();

        PopStyleColor(3);
        PopStyleVar(2);

        EndChild();
        SameLine(0, ScaleX(20));
        SetCursorPosY(GetCursorPosY() + ScaleY(3));
        Text(description.c_str());
    } else {
        std::string checkBoxMessage = " " + std::string(description);

        Checkbox(checkBoxMessage.c_str(), &state.isChecked);
        EndDisabled();
    }

    if (!tooltipText.empty()) {
        float autoUpdateColor = EaseInOutFloat(description, 0.f, 1.f, state.isHovered, 0.3f);

        if (autoUpdateColor) {
            SetMouseCursor(ImGuiMouseCursor_Hand);

            PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(ScaleX(10), ScaleY(10)));
            PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
            PushStyleVar(ImGuiStyleVar_Alpha, autoUpdateColor);
            PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.098f, 0.102f, 0.11f, 1.0f));

            SetTooltip(tooltipText.c_str());

            PopStyleVar(3);
            PopStyleColor();
        }

        state.isHovered = IsItemHovered() || (IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && IsMouseDown(ImGuiMouseButton_Left));
    }

    if (!endChild) {
        PopStyleColor(3);
        PopStyleVar(2);
    }

    return &state;
}