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

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include <stdio.h>
#include <SDL.h>
#include <misc/freetype/imgui_freetype.h>
#include <stdio.h>
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
#include <SDL_syswm.h>

using namespace ImGui;

int WINDOW_WIDTH  = 663;
int WINDOW_HEIGHT = 434;

const float TARGET_FPS = 240.0f;
const float TARGET_FRAME_TIME = 1.0f / TARGET_FPS;

std::atomic<bool> shouldSetupScaling {false};
SDL_Window* g_Window = nullptr;

void SetupImGuiScaling(SDL_Window* window, SDL_Renderer* renderer)
{
    SetupDPI(window);
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();
    SDL_Texture* fontTexture;
    unsigned char* pixels;
    int width, height;
    float scaleFactor = XDPI;
    constexpr static const auto fontPath = "C:/Windows/Fonts/seguiemj.ttf";
    static ImFontConfig cfg;
    static ImWchar ranges[] = { 0x1, 0x1FFFF, 0 };
    cfg.OversampleH = cfg.OversampleV = 1;
    cfg.MergeMode   = true;
    cfg.FontLoaderFlags |= ImGuiFreeTypeBuilderFlags_LoadColor;
   
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
    
    // Don't call Build() - the SDL2 renderer backend will handle font texture creation
    io.DisplayFramebufferScale = ImVec2(scaleFactor, scaleFactor);
    
    // Reset current ImGui style
    style = ImGuiStyle();
    style.ScaleAllSizes(scaleFactor);
    SetupColorScheme();
}

void SetupRenderer(SDL_Window* window, SDL_Renderer* renderer, std::shared_ptr<RouterNav> router)
{
    SetupColorScheme();
    SetupImGuiScaling(window, renderer);
    SetBorderlessWindowStyle(window, router);
    LoadTextures(renderer);
    
    // Initialize ImGui SDL2 backends
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);
}

void RunMainLoop(SDL_Window* window, SDL_Renderer* renderer, std::shared_ptr<RouterNav> router)
{
    const double TARGET_FRAME_TIME = 1.0 / 60.0; // 60 FPS
    bool running = true;
    bool hasShown = false;
    
    while (running) {
        Uint32 frameStartTime = SDL_GetTicks();
        
        // Handle events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            
            if (event.type == SDL_QUIT) {
                running = false;
            }
            else if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_CLOSE && 
                    event.window.windowID == SDL_GetWindowID(window)) {
                    running = false;
                }
            }
        }
        
        // Check for DPI scaling updates
        if (shouldSetupScaling.load(std::memory_order_relaxed)) {
            SetupImGuiScaling(window, renderer);
            shouldSetupScaling.store(false, std::memory_order_relaxed);
        }
        
        // Render frame
        RenderImGui(window, renderer, router);
        
        // Show window on first frame (equivalent to your hasShown logic)
        if (!hasShown) {
            SDL_ShowWindow(window);
            hasShown = true;
        }
        
        // Frame rate limiting
        Uint32 elapsedTime = SDL_GetTicks() - frameStartTime;
        Uint32 targetFrameTime = (Uint32)(TARGET_FRAME_TIME * 1000); // Convert to milliseconds
        
        if (elapsedTime < targetFrameTime) {
            SDL_Delay(targetFrameTime - elapsedTime);
        }
    }
}

// Alternative: If you really need threading for some reason
void SpawnRendererThread(SDL_Window* window, SDL_Renderer* renderer, std::shared_ptr<RouterNav> router)
{
    // Note: SDL2 renderer is NOT thread-safe by default
    // You should avoid this approach unless absolutely necessary
    
    SetupColorScheme();
    SetupImGuiScaling(window, renderer);
    SetBorderlessWindowStyle(window, router);
    LoadTextures(renderer);
    
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);
    
    const double TARGET_FRAME_TIME = 1.0 / 60.0;
    bool hasShown = false;
    
    while (true) {
        Uint32 frameStartTime = SDL_GetTicks();
        
        // Check if main thread wants to close
        // (You'd need to implement your own shutdown signaling)
        
        if (shouldSetupScaling.load(std::memory_order_relaxed)) {
            SetupImGuiScaling(window, renderer);
            shouldSetupScaling.store(false, std::memory_order_relaxed);
        }
        
        RenderImGui(window, renderer, router);
        
        if (!hasShown) {
            SDL_ShowWindow(window);
            hasShown = true;
        }
        
        Uint32 elapsedTime = SDL_GetTicks() - frameStartTime;
        Uint32 targetFrameTime = (Uint32)(TARGET_FRAME_TIME * 1000);
        
        if (elapsedTime < targetFrameTime) {
            SDL_Delay(targetFrameTime - elapsedTime);
        }
    }
}

// void setWindowIconFromMemory(GLFWwindow* window, const unsigned char* imageData, int width, int height) 
// {
//     GLFWimage icon[1];
//     icon[0].pixels = const_cast<unsigned char*>(imageData);
//     icon[0].width = width;
//     icon[0].height = height;

//     glfwSetWindowIcon(window, 1, icon);
// }


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);


    ::SetProcessDPIAware();

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");        // Best filtering
    // SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");                // Reduce tearing
    // SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");  

#ifdef _WIN32
    MMRESULT result = timeBeginPeriod(1);
    if (result != TIMERR_NOERROR) {
        std::cerr << "Failed to set timer resolution to 1 ms" << std::endl;
        return 1;
    }
#endif

    // Initialize SDL2
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        std::cerr << "Failed to initialize SDL2: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Get primary display info
    SDL_DisplayMode displayMode;
    if (SDL_GetCurrentDisplayMode(0, &displayMode) != 0) {
        std::cerr << "Failed to get display mode: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Create window (hidden initially)
    SDL_Window* window = SDL_CreateWindow(
        "Steam Homebrew",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_HIDDEN | SDL_WINDOW_ALLOW_HIGHDPI
    );
    
    if (!window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Create software renderer (no GPU required)
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Load and set window icon
    int width, height, channels;
    ColorScheme colorScheme = GetWindowsColorScheme();
    unsigned char* iconData = stbi_load_from_memory(
        colorScheme == Light ? windowIconDark : windowIconLight,
        colorScheme == Light ? sizeof(windowIconDark) : sizeof(windowIconLight),
        &width, &height, &channels, 4
    );
    
    if (iconData) {
        SDL_Surface* iconSurface = SDL_CreateRGBSurfaceFrom(
            iconData, width, height, 32, width * 4,
            0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000
        );
        if (iconSurface) {
            SDL_SetWindowIcon(window, iconSurface);
            SDL_FreeSurface(iconSurface);
        }
        stbi_image_free(iconData);
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.IniFilename = nullptr;

    // IMPORTANT: Setup fonts and styling BEFORE initializing backends
    SetupColorScheme();
    SetupImGuiScaling(window, renderer);
    
    int windowWidth = XDPI * WINDOW_WIDTH;
    int windowHeight = YDPI * WINDOW_HEIGHT;

    std::cout << fmt::format("Window size: {}x{}", windowWidth, windowHeight) << std::endl;

    SDL_SetWindowSize(window, windowWidth, windowHeight);

    int posX = (displayMode.w - ScaleX(WINDOW_WIDTH)) / 2;
    int posY = (displayMode.h - ScaleY(WINDOW_HEIGHT)) / 2;
    SDL_SetWindowPosition(window, posX, posY);

    // Initialize ImGui

    // Initialize ImGui SDL2 backends AFTER font setup
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    // Setup router
    std::shared_ptr<RouterNav> router = std::make_shared<RouterNav>(std::vector<Component>{ RenderHome });

    // Apply window styling and load textures
    SetBorderlessWindowStyle(window, router);
    LoadTextures(renderer);

    // Show window
    SDL_ShowWindow(window);
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            
            if (event.type == SDL_QUIT) {
                running = false;
            }
            else if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_CLOSE && 
                    event.window.windowID == SDL_GetWindowID(window)) {
                    running = false;
                }
            }
        }

        // Check for DPI scaling updates
        if (shouldSetupScaling.load(std::memory_order_relaxed)) {
            // Shutdown backends before font changes
            ImGui_ImplSDLRenderer2_Shutdown();
            ImGui_ImplSDL2_Shutdown();
            
            // Update fonts and scaling
            SetupImGuiScaling(window, renderer);
            
            // Reinitialize backends after font changes
            ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
            ImGui_ImplSDLRenderer2_Init(renderer);
            
            shouldSetupScaling.store(false, std::memory_order_relaxed);
        }

        // Start ImGui frame
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Render your UI
        RenderImGui(window, renderer, router);

        // Rendering
        ImGui::Render();
        SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

#ifdef _WIN32
    timeEndPeriod(1);
#endif

    return 0;
}
