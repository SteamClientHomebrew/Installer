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

#pragma once

#include <memory>
#include <router.h>
#include <nlohmann/json.hpp>

bool RenderTitleBarComponent(std::shared_ptr<RouterNav> router);
const void RenderHome(std::shared_ptr<RouterNav> router, float xPos);
const void RenderInstallPrompt(std::shared_ptr<RouterNav> router, float xPos);
const void RenderInstaller(std::shared_ptr<RouterNav> router, float xPos);
const void RenderUninstallSelect(std::shared_ptr<RouterNav> router, float xPos);

const void RenderBottomNavBar(const char* identifier, float xPos, std::function<void()> buttonRenderCallback);

void StartInstaller(std::string steamPath, nlohmann::json& releaseInfo, nlohmann::json& osReleaseInfo);
void InitializeUninstaller();
const bool FetchVersionInfo();

enum MessageLevel
{
    Error,
    Info,
    Warning
};

void ShowMessageBox(std::string title, std::string body, MessageLevel level);
void RenderMessageBoxes();

struct CheckBoxState
{
    bool isHovered;
    bool isChecked;

    CheckBoxState(bool isChecked) : isHovered(false), isChecked(false)
    {
        this->isChecked = isChecked;
    }
};

const CheckBoxState* RenderCheckBox(bool checked, const char* description, const char* tooltipText = nullptr, bool disabled = false, bool endChild = false);