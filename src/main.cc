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

#define GL_SILENCE_DEPRECATION
#define GLFW_EXPOSE_NATIVE_WIN32

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <misc/freetype/imgui_freetype.h>
#include <stdio.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <wtypes.h>
#include <memory.h>
#include <atomic>
#include <animate.h>
#include <texture.h>
#include <filesystem>
#include <router.h>
#include <iostream>
#include <algorithm>
#include <theme.h>
#include <wndproc.h>
#include <imspinner.h>
#include <thread>
#include <components.h>
#include <dpi.h>
#include <renderer.h>
#include <fmt/format.h>
#include <stb_image.h>
#include <util.h>
#include <dwmapi.h>

using namespace ImGui;

int WINDOW_WIDTH  = 663;
int WINDOW_HEIGHT = 434;

const float TARGET_FPS = 240.0f;
const float TARGET_FRAME_TIME = 1.0f / TARGET_FPS;

static void GLFWErrorCallback(int error, const char* description)
{
    if (error == GLFW_API_UNAVAILABLE) 
    {
        MessageBoxA(NULL, "OpenGL was not found on your system, and cannot proceed to render. Download mesa3d-25.1.3-release-mingw.7z from https://github.com/pal1000/mesa-dist-win/releases and placing it in the same directory as this installer exe.", "Error", MB_ICONERROR);
        return;
    }

    MessageBoxA(NULL, fmt::format("An error occurred.\n\nGLFW Error {}: {}", error, description).c_str(), "Whoops!", MB_ICONERROR);
}

void WindowRefreshCallback(GLFWwindow *window)
{
    glfwSwapBuffers(window);
    glFinish();
}

std::atomic<bool> shouldSetupScaling {false};
GLFWwindow* g_Window = nullptr;

void SetupImGuiScaling(GLFWwindow* window) 
{
    SetupDPI(window);
    ImGuiIO& io = GetIO();
    ImGuiStyle& style = GetStyle();
    GLuint fontTexture;

    unsigned char* pixels;
    int width, height;
    float scaleFactor = XDPI;

    constexpr static const auto fontPath = "C:/Windows/Fonts/seguiemj.ttf";
    static ImFontConfig cfg;
    static ImWchar ranges[] = { 0x1, 0x1FFFF, 0 };

    cfg.OversampleH = cfg.OversampleV = 1;
    cfg.MergeMode   = true;
    cfg.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_LoadColor;
    
    // Clear fonts
    io.Fonts->Clear();
    io.Fonts->AddFontFromMemoryTTF((void*)GeistVaraible, sizeof(GeistVaraible), 16.0f * scaleFactor);
    if (std::filesystem::exists(fontPath)) 
    {
        io.Fonts->AddFontFromFileTTF(fontPath, 12.0f * scaleFactor, &cfg, ranges);
    }
    io.Fonts->AddFontFromMemoryTTF((void*)Geist_Bold,    sizeof(Geist_Bold),    18.0f * scaleFactor);
    if (std::filesystem::exists(fontPath)) 
    {
        io.Fonts->AddFontFromFileTTF(fontPath, 16.0f * scaleFactor, &cfg, ranges);
    }

    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    glGenTextures  ( 1, &fontTexture );
    glBindTexture  ( GL_TEXTURE_2D, fontTexture );
    glTexImage2D   ( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

    io.Fonts->SetTexID((ImTextureID)(intptr_t)fontTexture);
    io.DisplayFramebufferScale = ImVec2(scaleFactor, scaleFactor);

    /** Reset current ImGui style */
    style = ImGuiStyle(); 
    style.ScaleAllSizes(scaleFactor);
    SetupColorScheme();
}

void FrameBufferSizeCallback(GLFWwindow* window, int width, int height) 
{
    g_Window = window;
    shouldSetupScaling.store(true, std::memory_order_relaxed);
}

void SpawnRendererThread(GLFWwindow* window, const char* glsl_version, std::shared_ptr<RouterNav> router)
{
    glfwMakeContextCurrent(window); 
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) 
    {
        std::cerr << "Failed to initialize GLAD\n";
        return;
    }

    SetupColorScheme();
    SetupImGuiScaling(window);
    SetBorderlessWindowStyle(window, router);
    LoadTextures();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    while (!glfwWindowShouldClose(window)) 
    {
        double frameStartTime = glfwGetTime();

        if (shouldSetupScaling.load(std::memory_order_relaxed)) 
        {
            SetupImGuiScaling(window);
            shouldSetupScaling.store(false, std::memory_order_relaxed);
        }

        RenderImGui(window, router);

        static bool hasShown = false;

        if (!hasShown)
        {
            glfwShowWindow(window);
            hasShown = true;
        }

        double elapsedTime   = glfwGetTime() - frameStartTime;
        double remainingTime = TARGET_FRAME_TIME - elapsedTime;

        if (remainingTime > 0) 
        {
            Sleep((DWORD)(remainingTime * 1000));  
        }
    }
}

void setWindowIconFromMemory(GLFWwindow* window, const unsigned char* imageData, int width, int height) 
{
    GLFWimage icon[1];
    icon[0].pixels = const_cast<unsigned char*>(imageData);
    icon[0].width = width;
    icon[0].height = height;

    glfwSetWindowIcon(window, 1, icon);
}

void RenderBlur( HWND hwnd ) 
{
    struct ACCENTPOLICY 
    {
        int na;
        int nf;
        int nc;
        int nA;
    };
    
    struct WINCOMPATTRDATA 
    {
        int na;
        PVOID pd;
        ULONG ul;
    };

    const HINSTANCE user32Handle = LoadLibrary( "user32.dll" );

    if (user32Handle) 
    {
        typedef BOOL( WINAPI* pSetWindowCompositionAttribute )( HWND, WINCOMPATTRDATA* );
        const pSetWindowCompositionAttribute SetWindowCompositionAttribute = (pSetWindowCompositionAttribute)GetProcAddress(user32Handle, "SetWindowCompositionAttribute");

        if ( SetWindowCompositionAttribute ) 
        {
            ACCENTPOLICY policy = { 3, 0, 0, 0 };
            WINCOMPATTRDATA data = { 19, &policy,sizeof(ACCENTPOLICY) };
            SetWindowCompositionAttribute(hwnd, &data);
        }
        FreeLibrary(user32Handle);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    if (IsDebuggerPresent())
    {
        AllocConsole();
        FILE* file;
        freopen_s(&file, "CONOUT$", "w", stdout);
        freopen_s(&file, "CONOUT$", "w", stderr);
    }

    MMRESULT result = timeBeginPeriod(1);

    if (result != TIMERR_NOERROR) {
        std::cerr << "Failed to set timer resolution to 1 ms" << std::endl;
        return 1;
    }

    glfwSetErrorCallback(GLFWErrorCallback);
    if (!glfwInit())
    {
        MessageBoxA(NULL, "Failed to initialize GLFW", "Error", MB_ICONERROR);
        return 1;
    }

    GLFWmonitor* monitor = glfwGetPrimaryMonitor(); 
    if (!monitor) 
    {
        MessageBoxA(NULL, "Failed to get primary monitor", "Error", MB_ICONERROR);
        glfwTerminate();
        return -1;
    }

    const char* glsl_version = "#version 130";
    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 0 );
    glfwWindowHint( GLFW_SCALE_TO_MONITOR,      GLFW_TRUE  );
    glfwWindowHint( GLFW_RESIZABLE,             GLFW_FALSE );
    glfwWindowHint( GLFW_VISIBLE,               GLFW_FALSE );
    glfwWindowHint( GLFW_SAMPLES,               4 ); 

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Steam Homebrew", nullptr, nullptr);
    if (window == nullptr)
    {
        return 1;
    }
    
    SetupDPI(window);

    #ifdef _WIN32
    HWND hwnd = glfwGetWin32Window(window);
    RenderBlur(hwnd);
    #endif

    // Load image from memory
    int width, height, channels;
    ColorScheme colorScheme = GetWindowsColorScheme();

    unsigned char* data = stbi_load_from_memory(
        colorScheme ==  Light ? windowIconDark : windowIconLight, 
        colorScheme ==  Light ? sizeof(windowIconDark) : sizeof(windowIconLight), 
        &width, &height, &channels, 4
    ); 

    if (data)
    {
        setWindowIconFromMemory(window, data, width, height);
        stbi_image_free(data);
    }

    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    if (!primaryMonitor) 
    {
        std::cerr << "Failed to get primary monitor\n";
        glfwTerminate();
        return -1;
    }

    int monitorX, monitorY;
    glfwGetMonitorPos(primaryMonitor, &monitorX, &monitorY);

    const GLFWvidmode* vidMode = glfwGetVideoMode(primaryMonitor);
    if (!vidMode) 
    {
        std::cerr << "Failed to get video mode\n";
        glfwTerminate();
        return -1;
    }

    int posX = monitorX + (vidMode->width - (WINDOW_WIDTH * XDPI)) / 2;
    int posY = monitorY + (vidMode->height - (WINDOW_HEIGHT * YDPI)) / 2;

    glfwSetWindowPos(window, posX, posY);

    glfwSetWindowRefreshCallback  (window, WindowRefreshCallback);
    glfwSetFramebufferSizeCallback(window, FrameBufferSizeCallback);

    IMGUI_CHECKVERSION();
    CreateContext();
    ImGuiIO& io = GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.IniFilename = nullptr;

    std::shared_ptr<RouterNav> router = std::make_shared<RouterNav>(std::vector<Component>{ RenderHome });
    auto rendererThread = std::thread(SpawnRendererThread, window, glsl_version, router);

    while (!glfwWindowShouldClose(window))
    {
        glfwWaitEvents();
    }

    rendererThread.join();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
