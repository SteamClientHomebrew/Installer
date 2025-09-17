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

#include "components.h"
#include <queue>
#include <imgui.h>
#include <dpi.h>
#include <animate.h>
#include <imgui_internal.h>

using namespace ImGui;

struct MessageBoxProps
{
    std::string title;
    std::string body;
    MessageLevel level;
};

std::queue<MessageBoxProps> messageBoxQueue;

/**
 * Show a message box with the given title, body, and level.
 * The message box will be added to the queue and displayed when RenderMessageBoxes() is called.
 */
void ShowMessageBox(std::string title, std::string body, MessageLevel level)
{
    messageBoxQueue.push({ title, body, level });
}

/**
 * Render the message boxes in the queue.
 * This function will create a popup for each message box in the queue.
 * The message boxes will be displayed one at a time, and the user can close them by clicking outside the popup or pressing the "Continue" button.
 * The function will also handle the scrolling of the message box content if it exceeds the window size.
 */
void RenderMessageBoxes()
{
    if (messageBoxQueue.size() <= 0) {
        return;
    }

    OpenPopup("MessageAlertPopup");

    const ImGuiIO& io = GetIO();
    const MessageBoxProps currentBox = messageBoxQueue.front();

    SetNextWindowBgAlpha(1.f);
    static bool hasSkippedFirstFrame = false;

    static float scrollPosition = 0.0f;
    static float scrollVelocity = 0.0f;
    static const float scrollSpeed = 10.0f;
    static const float scrollFriction = 0.95f;
    static const float minVelocityThreshold = 0.01f;

    float opacityAnimation = EaseInOutFloat("MessageAlertPopupAnimation", 0.f, 1.f, true, 0.3f);

    PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.098f, 0.102f, 0.11f, 1.0f));
    PushStyleColor(ImGuiCol_Border, ImVec4(0.48f, 0.484f, 0.492f, 0.5f));
    PushStyleVar(ImGuiStyleVar_Alpha, opacityAnimation);
    PushStyleVar(ImGuiStyleVar_WindowRounding, ScaleX(10.0f));
    PushStyleVar(ImGuiStyleVar_WindowBorderSize, ScaleX(1.0f));

    SetNextWindowSizeConstraints(ImVec2(ScaleX(500), 0), ImVec2(ScaleX(700), FLT_MAX));

    if (BeginPopupModal("MessageAlertPopup", nullptr,
                        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
                            ImGuiWindowFlags_NoScrollWithMouse)) {
        if (io.MouseWheel != 0.0f) {
            scrollVelocity -= io.MouseWheel * scrollSpeed;
        }

        scrollVelocity *= scrollFriction;
        scrollPosition += scrollVelocity;

        const auto cursorPos = GetCursorPosY();

        PushFont(io.Fonts->Fonts[1]);
        /** Render icon based on message level */
        switch (currentBox.level) {
            case MessageLevel::Info:
                Text("ℹ️");
                break;
            case MessageLevel::Warning:
                Text("⚠️");
                break;
            case MessageLevel::Error:
                Text("❌");
                break;
        }
        SameLine(0, ScaleX(-10));

        TextWrapped(currentBox.title.c_str());
        PopFont();
        SetCursorPosY(GetCursorPosY() + ScaleY(15));
        Separator();
        SetCursorPosY(GetCursorPosY() + ScaleY(20));

        TextWrapped(currentBox.body.c_str());

        scrollPosition = ImClamp(scrollPosition, 0.0f, GetScrollMaxY());

        if (fabs(scrollVelocity) < minVelocityThreshold) {
            scrollVelocity = 0.0f;
        }

        SetScrollY(scrollPosition);
        if (hasSkippedFirstFrame && !ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) {
            CloseCurrentPopup();
            messageBoxQueue.pop();
        }
        hasSkippedFirstFrame = true;

        SetCursorPosY(GetCursorPosY() + ScaleY(40));

        PushStyleColor(ImGuiCol_Button, ImVec4(1.f, 1.f, 1.f, 1.f));
        PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.f, 1.f, 1.f, 1.f));
        PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 0.f, 0.f, 1.f));

        SetCursorPosX(GetCursorPosX() + GetContentRegionAvail().x - ScaleX(140));

        if (Button("Continue", ImVec2(ScaleX(140), ScaleY(50)))) {
            CloseCurrentPopup();
            messageBoxQueue.pop();
        }

        PopStyleColor(3);
        EndPopup();
    }

    PopStyleColor(2);
    PopStyleVar(3);
}