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
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <router.h>
#include <renderer.h>
#include <wndproc.h>
#include <dpi.h>
#include <components.h>
#include <iostream>
#include <thread>
#include "updater.h"

void AllocateDeveloperConsoleIfNeeded()
{
    if (!IsDebuggerPresent()) {
        return;
    }

    if (!AllocConsole()) {
        return;
    }

    FILE* file;
    freopen_s(&file, "CONOUT$", "w", stdout);
    freopen_s(&file, "CONOUT$", "w", stderr);
}

int RunInstallerWindow(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.IniFilename = nullptr;

    std::shared_ptr<RouterNav> router = std::make_shared<RouterNav>(std::vector<Component>{ RenderHome });
    auto rendererThread = std::thread(SpawnRendererThread, window, "#version 130", router);

    while (!glfwWindowShouldClose(window)) {
        glfwWaitEvents();
    }

    rendererThread.join();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

GLFWwindow* CreateWindowContext(GLFWmonitor* monitor)
{
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Steam Homebrew", nullptr, nullptr);
    if (window == nullptr) {
        glfwTerminate();
        return 0;
    }

    SetupDPI(window);
    SetWindowIcon(window);

    const GLFWvidmode* vidMode = glfwGetVideoMode(monitor);
    if (!vidMode) {
        MessageBoxA(NULL, "Failed to get video mode", "Error", MB_ICONERROR);
        glfwDestroyWindow(window);
        glfwTerminate();
        return 0;
    }

    int monitorX, monitorY;
    glfwGetMonitorPos(monitor, &monitorX, &monitorY);
    int posX = monitorX + (vidMode->width - (WINDOW_WIDTH * XDPI)) / 2;
    int posY = monitorY + (vidMode->height - (WINDOW_HEIGHT * YDPI)) / 2;
    glfwSetWindowPos(window, posX, posY);

    glfwSetWindowRefreshCallback(window, WindowRefreshCallback);
    glfwSetFramebufferSizeCallback(window, FrameBufferSizeCallback);
    return window;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    AllocateDeveloperConsoleIfNeeded();
    CheckForAndDownloadUpdates();

    MMRESULT result = timeBeginPeriod(1);
    /* not fatal, just worse performance in the app */
    if (result != TIMERR_NOERROR) {
        std::cerr << "Failed to set timer resolution to 1 ms" << std::endl;
    }

    glfwSetErrorCallback(GLFWErrorCallback);
    if (!glfwInit()) {
        MessageBoxA(NULL, "Failed to initialize GLFW", "Error", MB_ICONERROR);
        return 1;
    }

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (!monitor) {
        MessageBoxA(NULL, "Failed to get primary monitor", "Error", MB_ICONERROR);
        glfwTerminate();
        return 1;
    }

    GLFWwindow* window = CreateWindowContext(monitor);
    if (!window) {
        return 1;
    }

    return RunInstallerWindow(window);
}
