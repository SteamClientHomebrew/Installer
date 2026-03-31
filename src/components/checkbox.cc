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
#include <unordered_map>
#include <animate.h>

using namespace ImGui;

void RenderCheckMarkPop(ImDrawList* draw_list, ImVec2 pos, ImU32 col, float sz, float progress)
{
    ImVec2 p0 = pos + ImVec2(0.2f * sz, 0.5f * sz);
    ImVec2 p1 = pos + ImVec2(0.4f * sz, 0.7f * sz);
    ImVec2 p2 = pos + ImVec2(0.8f * sz, 0.3f * sz);

    if (progress < 0.5f) {
        float t = progress / 0.5f;
        ImVec2 mid = p0 + (p1 - p0) * t;
        draw_list->AddLine(p0, mid, col, 2.0f);
    } else {
        draw_list->AddLine(p0, p1, col, 2.0f);
        float t = (progress - 0.5f) / 0.5f;
        ImVec2 mid = p1 + (p2 - p1) * t;
        draw_list->AddLine(p1, mid, col, 2.0f);
    }
}

/**
 * override Imgui's internal implementation of the checkbox as it sucks.
 * requires /FORCE:MULTIPLE on MSVC.
 */
bool ImGui::Checkbox(const char* label, bool* v)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

    const float square_sz = ImGui::GetFrameHeight();
    const ImVec2 pos = window->DC.CursorPos;
    const ImRect total_bb(pos, pos + ImVec2(square_sz + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), label_size.y + style.FramePadding.y * 2.0f));
    ImGui::ItemSize(total_bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(total_bb, id))
        return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
    if (pressed) {
        *v = !*v;
        ImGui::MarkItemEdited(id);
    }

    static std::unordered_map<ImGuiID, float> g_CheckboxAnimProgress;
    auto it = g_CheckboxAnimProgress.find(id);
    if (it == g_CheckboxAnimProgress.end()) {
        g_CheckboxAnimProgress[id] = *v ? 1.0f : 0.0f;
    }
    float& progress = g_CheckboxAnimProgress[id];
    float speed = 6.0f;
    progress += (*v ? 1.0f : -1.0f) * speed * g.IO.DeltaTime;
    progress = ImClamp(progress, 0.0f, 1.0f);

    ImRect check_bb(pos, pos + ImVec2(square_sz, square_sz));
    ImU32 frame_col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
    window->DrawList->AddRectFilled(check_bb.Min, check_bb.Max, frame_col, style.FrameRounding);

    if (progress > 0.0f) {
        ImU32 check_col = ImGui::GetColorU32(ImGuiCol_CheckMark);
        const float pad = ImMax(1.0f, IM_TRUNC(square_sz / 6.0f));
        ImVec2 anim_pos = check_bb.Min + ImVec2(pad, pad);
        float size = square_sz - pad * 2.0f;
        RenderCheckMarkPop(window->DrawList, anim_pos, check_col, size, progress);
    }

    const ImVec2 label_pos = ImVec2(check_bb.Max.x + style.ItemInnerSpacing.x, check_bb.Min.y + style.FramePadding.y);
    if (label_size.x > 0.0f)
        ImGui::RenderText(label_pos, label);

    return pressed;
}

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
        Text("%s", description.c_str());
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

            SetTooltip("%s", tooltipText.c_str());

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
