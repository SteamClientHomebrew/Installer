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

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <renderer.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <misc/freetype/imgui_freetype.h>
#include <components.h>
#include <wndproc.h>
#include <texture.hh>
#include <theme.h>
#include <dpi.h>
#include <memory.h>
#include <filesystem>
#include <atomic>
#include <iostream>
#include <format>
#include <windows.h>

using namespace ImGui;

int WINDOW_WIDTH = 663;
int WINDOW_HEIGHT = 434;

static const float TARGET_FPS = 240.0f;
static const float TARGET_FRAME_TIME = 1.0f / TARGET_FPS;

static std::atomic<bool> shouldSetupScaling{ false };
static GLFWwindow* g_Window = nullptr;

void GLFWErrorCallback(int error, const char* description)
{
    if (error == GLFW_API_UNAVAILABLE) {
        MessageBoxA(NULL,
            "Your system doesn't support hardware rendering, which is preventing the installer from displaying properly. "
            "To resolve this issue, download the DLL files from:\n"
            "https://github.com/pal1000/mesa-dist-win/releases/latest\n"
            "Download mesa3d-x.x.x-release-msvc.7z and place them in the same folder as the installer. "
            "This will enable software rendering as an alternative.\n\n"
            "Please note: This is a system limitation, not an installer bug, so there's no need to report it to the development team.",
            "Error", MB_ICONERROR);
        return;
    }

    MessageBoxA(NULL, std::format("An error occurred.\n\nGLFW Error {}: {}", error, description).c_str(), "Whoops!", MB_ICONERROR);
}

void WindowRefreshCallback(GLFWwindow* window)
{
    glfwSwapBuffers(window);
    glFinish();
}

void FrameBufferSizeCallback(GLFWwindow* window, int width, int height)
{
    g_Window = window;
    shouldSetupScaling.store(true, std::memory_order_relaxed);
}

void SetupImGuiScaling(GLFWwindow* window)
{
    SetupDPI(window);

    ImGuiIO& io = GetIO();
    ImGuiStyle& style = GetStyle();
    float scaleFactor = XDPI;

    constexpr static const auto fontPath = "C:/Windows/Fonts/seguiemj.ttf";
    static ImFontConfig cfg;

    cfg.MergeMode = true;
    cfg.FontLoaderFlags |= ImGuiFreeTypeLoaderFlags_LoadColor;

    ImFontConfig mem_cfg;
    mem_cfg.FontDataOwnedByAtlas = false;

    io.Fonts->Clear();
    io.Fonts->AddFontFromMemoryTTF((void*)GeistVariable, sizeof(GeistVariable), 16.0f * scaleFactor, &mem_cfg);
    if (std::filesystem::exists(fontPath))
        io.Fonts->AddFontFromFileTTF(fontPath, 14.0f * scaleFactor, &cfg);

    io.Fonts->AddFontFromMemoryTTF((void*)Geist_Bold, sizeof(Geist_Bold), 18.0f * scaleFactor, &mem_cfg);
    if (std::filesystem::exists(fontPath))
        io.Fonts->AddFontFromFileTTF(fontPath, 14.0f * scaleFactor, &cfg);

    /** Explicitly set FreeType as the font loader to ensure color emoji support */
    io.Fonts->SetFontLoader(ImGuiFreeType::GetFontLoader());
    io.Fonts->Build();

    io.DisplayFramebufferScale = ImVec2(scaleFactor, scaleFactor);

    /** Reset current ImGui style */
    style = ImGuiStyle();
    style.ScaleAllSizes(scaleFactor);
    SetupColorScheme();
}

void SpawnRendererThread(GLFWwindow* window, const char* glsl_version, std::shared_ptr<RouterNav> router)
{
    glfwMakeContextCurrent(window);
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW\n";
        return;
    }

    SetupColorScheme();
    SetupImGuiScaling(window);
    SetBorderlessWindowStyle(window, router);
    LoadTextures();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    while (!glfwWindowShouldClose(window)) {
        double frameStartTime = glfwGetTime();

        if (shouldSetupScaling.load(std::memory_order_relaxed)) {
            SetupImGuiScaling(window);
            shouldSetupScaling.store(false, std::memory_order_relaxed);
        }

        RenderImGui(window, router);

        static bool hasShown = false;

        if (!hasShown) {
            glfwShowWindow(window);
            hasShown = true;
        }

        double elapsedTime = glfwGetTime() - frameStartTime;
        double remainingTime = TARGET_FRAME_TIME - elapsedTime;

        if (remainingTime > 0) {
            Sleep((DWORD)(remainingTime * 1000));
        }
    }
}

void RenderImGui(GLFWwindow* window, std::shared_ptr<RouterNav> router)
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    NewFrame();

    ImGuiViewport* viewport = GetMainViewport();
    {
        SetNextWindowPos(viewport->Pos);
        SetNextWindowSize(viewport->Size);

        PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        Begin("Millennium", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        PopStyleVar();

        isTitleBarHovered = RenderTitleBarComponent(router);
        router->update();

        auto currentPanel = router->getCurrentComponent();
        auto transitioningPanel = router->getTransitioningComponent();
        auto xOffsetCurrent = router->getCurrentOffset(viewport->Size.x);
        auto xOffsetTransitioning = router->getTransitioningOffset(viewport->Size.x);

        if (transitioningPanel) {
            PushID("TransitioningPanel");
            transitioningPanel(router, xOffsetTransitioning);
            PopID();
        }

        SameLine();
        currentPanel(router, xOffsetCurrent);

        if (IsKeyPressed(ImGuiKey_MouseX1)) {
            if (router->canGoBack())
                router->navigateBack();
        }

        End();
        RenderMessageBoxes();
    }
    Render();

    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);

    ImVec4 clear_color = ImVec4(0.f, 0.f, 0.f, 0.f);

    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(GetDrawData());
    glfwSwapBuffers(window);
}
