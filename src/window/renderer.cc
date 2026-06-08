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
#include <i18n.h>
#include <cjk_names.h>
#include <viet_name.h>
#include <memory.h>
#include <filesystem>
#include <atomic>
#include <iostream>
#include <format>
#include <thread>
#ifdef _WIN32
#include <windows.h>
#endif

using namespace ImGui;

int WINDOW_WIDTH = 663;
int WINDOW_HEIGHT = 434;

static const float TARGET_FPS = 240.0f;
static const float TARGET_FRAME_TIME = 1.0f / TARGET_FPS;

static std::atomic<bool> shouldSetupScaling{ false };
static GLFWwindow* g_Window = nullptr;

void GLFWErrorCallback(int error, const char* description)
{
#ifdef _WIN32
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
#else
    std::cerr << std::format("GLFW Error {}: {}", error, description) << std::endl;
#endif
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

void RequestFontRebuild()
{
    shouldSetupScaling.store(true, std::memory_order_relaxed);
}

void SetupImGuiScaling(GLFWwindow* window)
{
    SetupDPI(window);

    ImGuiIO& io = GetIO();
    ImGuiStyle& style = GetStyle();
    float scaleFactor = XDPI;

    ImFontConfig mem_cfg;
    mem_cfg.FontDataOwnedByAtlas = false;

    // ── CJK embedded name-only fonts ─────────────────────────────────────────
    // Always merged: tiny subsets (~21 KB total) covering only the characters
    // used in language-selector display names (简体中文 繁體中文 日本語 한국어).
    ImFontConfig cjk_name_cfg;
    cjk_name_cfg.MergeMode           = true;
    cjk_name_cfg.FontDataOwnedByAtlas = false;

    static const ImWchar cjk_ideograph_ranges[] = {
        0x4E00, 0x9FFF,  // CJK Unified Ideographs (covers all 9 name chars)
        0,
    };
    static const ImWchar cjk_hangul_ranges[] = {
        0xAC00, 0xD7AF,  // Hangul Syllables (한국어)
        0,
    };

    // ── System CJK font for full UI rendering when a CJK locale is active ────
    // Searched in platform-specific locations; gracefully skipped if absent.
    static const ImWchar cjk_full_ranges[] = {
        0x3040, 0x30FF,  // Hiragana + Katakana
        0x4E00, 0x9FFF,  // CJK Unified Ideographs
        0xAC00, 0xD7AF,  // Hangul Syllables
        0xFF00, 0xFFEF,  // Halfwidth/Fullwidth Forms
        0,
    };

    // Returns the first existing path from a list.
    auto findFont = [](std::initializer_list<const char*> candidates) -> std::string {
        for (const char* p : candidates)
            if (p && std::filesystem::exists(p)) return p;
        return {};
    };

#if !defined(_WIN32)
    // Ask fontconfig for the best font for a given language tag.
    // Available on all major Linux distros; on macOS requires Homebrew fontconfig.
    // Returns empty string if fc-match is not installed or returns a Latin fallback.
    auto fcMatch = [](const char* langTag) -> std::string {
        std::string cmd = std::string("fc-match --format='%{file}' ':lang=") + langTag + "' 2>/dev/null";
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return {};
        char buf[1024] = {};
        if (!fgets(buf, sizeof(buf), pipe)) { pclose(pipe); return {}; }
        pclose(pipe);
        std::string path(buf);
        while (!path.empty() && (path.back() == '\n' || path.back() == '\r' || path.back() == ' '))
            path.pop_back();
        // Reject results that are just the Latin fallback (e.g. DejaVuSans) —
        // fontconfig always returns *something* even when no CJK font is installed.
        if (path.find("DejaVu") != std::string::npos) return {};
        if (path.find("LiberationSans") != std::string::npos) return {};
        if (path.find("FreeSans") != std::string::npos) return {};
        return std::filesystem::exists(path) ? path : std::string{};
    };
#endif

    const std::string cjkSystemFont = [&]() -> std::string {
        const std::string& lang = Locale::GetCurrentLanguageId();
        if (!Locale::UsesCJK()) return {};

#if defined(_WIN32)
        if (lang == "schinese")
            return findFont({ "C:/Windows/Fonts/msyh.ttc", "C:/Windows/Fonts/simsun.ttc" });
        if (lang == "tchinese")
            return findFont({ "C:/Windows/Fonts/msjh.ttc", "C:/Windows/Fonts/msyh.ttc" });
        if (lang == "japanese")
            return findFont({ "C:/Windows/Fonts/YuGothL.ttc", "C:/Windows/Fonts/msgothic.ttc" });
        if (lang == "koreana")
            return findFont({ "C:/Windows/Fonts/malgun.ttf" });
        return {};

#elif defined(__APPLE__)
        // macOS: try fontconfig first (available via Homebrew), then fall back to
        // known system font paths. PingFang covers both Chinese variants.
        // Hiragino Sans GB covers Japanese (ASCII filename, ships with macOS).
        if (lang == "schinese" || lang == "tchinese") {
            auto r = fcMatch("zh");
            return r.empty() ? findFont({
                "/System/Library/Fonts/PingFang.ttc",
                "/Library/Fonts/Arial Unicode MS.ttf"
            }) : r;
        }
        if (lang == "japanese") {
            auto r = fcMatch("ja");
            return r.empty() ? findFont({
                "/System/Library/Fonts/Hiragino Sans GB.ttc",
                "/Library/Fonts/Arial Unicode MS.ttf"
            }) : r;
        }
        if (lang == "koreana") {
            auto r = fcMatch("ko");
            return r.empty() ? findFont({
                "/System/Library/Fonts/AppleSDGothicNeo.ttc",
                "/Library/Fonts/Arial Unicode MS.ttf"
            }) : r;
        }
        return {};

#else
        // Linux: fontconfig is virtually always available.
        if (lang == "schinese" || lang == "tchinese") return fcMatch("zh");
        if (lang == "japanese")                        return fcMatch("ja");
        if (lang == "koreana")                         return fcMatch("ko");
        return {};
#endif
    }();

    // ── Vietnamese system fonts (Arial/Liberation – loaded as primary) ────────
    // When Vietnamese is active:
    //   Fonts[0] = Arial Regular (primary, full coverage — no Geist mixing)
    //   Fonts[1] = Arial Bold    (primary)
    //   Fonts[3] = Geist + CJK names — pushed explicitly in the dropdown so
    //              it keeps looking the same as English mode.
    const std::string vietSystemFont = [&]() -> std::string {
        if (Locale::GetCurrentLanguageId() != "vietnamese") return {};
#if defined(_WIN32)
        return findFont({ "C:/Windows/Fonts/arial.ttf" });
#elif defined(__APPLE__)
        // fcMatch works when fontconfig is installed (e.g. via Homebrew).
        // HelveticaNeue and LucidaGrande ship with every macOS and cover
        // Latin Extended Additional (all Vietnamese diacritics).
        { auto r = fcMatch("vi"); if (!r.empty()) return r; }
        return findFont({
            "/Library/Fonts/Arial.ttf",
            "/System/Library/Fonts/Supplemental/Arial.ttf",
            "/System/Library/Fonts/HelveticaNeue.ttc",
            "/System/Library/Fonts/LucidaGrande.ttc",
        });
#else
        return findFont({
            "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
            "/usr/share/fonts/truetype/msttcorefonts/Arial.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        });
#endif
    }();

    const std::string vietSystemFontBold = [&]() -> std::string {
        if (Locale::GetCurrentLanguageId() != "vietnamese") return {};
#if defined(_WIN32)
        return findFont({ "C:/Windows/Fonts/arialbd.ttf" });
#elif defined(__APPLE__)
        return findFont({
            "/Library/Fonts/Arial Bold.ttf",
            "/System/Library/Fonts/Supplemental/Arial Bold.ttf",
            // HelveticaNeue.ttc Bold is at a non-zero TTC index — can't pick it
            // without FontNo; falls back to vietSystemFont (Regular) below.
        });
#else
        return findFont({
            "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
            "/usr/share/fonts/truetype/msttcorefonts/Arial_Bold.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        });
#endif
    }();

    const bool isVietnamese = !vietSystemFont.empty();

    // Ranges for the 2 glyphs missing from Geist, used in the Geist dropdown
    // font (Fonts[3]) so the "Tieng Viet" preview renders without boxes.
    static const ImWchar viet_preview_ranges[] = {
        0x1EBF, 0x1EBF,  // ế
        0x1EC7, 0x1EC7,  // ệ
        0,
    };
    ImFontConfig viet_preview_cfg;
    viet_preview_cfg.MergeMode = true;
    viet_preview_cfg.FontDataOwnedByAtlas = false;

    io.Fonts->Clear();

    /** Fonts[0] – UI regular: Arial when Vietnamese, Geist otherwise */
    if (isVietnamese) {
        io.Fonts->AddFontFromFileTTF(vietSystemFont.c_str(), 16.0f * scaleFactor, nullptr);
    } else {
        io.Fonts->AddFontFromMemoryTTF((void*)GeistVariable, sizeof(GeistVariable), 16.0f * scaleFactor, &mem_cfg);
        io.Fonts->AddFontFromMemoryTTF((void*)CJKNames_Ideographs, sizeof(CJKNames_Ideographs), 16.0f * scaleFactor, &cjk_name_cfg, cjk_ideograph_ranges);
        io.Fonts->AddFontFromMemoryTTF((void*)CJKNames_Korean, sizeof(CJKNames_Korean), 16.0f * scaleFactor, &cjk_name_cfg, cjk_hangul_ranges);
        if (!cjkSystemFont.empty())
            io.Fonts->AddFontFromFileTTF(cjkSystemFont.c_str(), 16.0f * scaleFactor, &cjk_name_cfg, cjk_full_ranges);
    }
    if (std::filesystem::exists(fontPath))
        io.Fonts->AddFontFromFileTTF(fontPath, 14.0f * scaleFactor, &cfg);
#endif

    /** Fonts[1] – UI bold: Arial Bold when Vietnamese, Geist Bold otherwise */
    if (isVietnamese) {
        const std::string& boldSrc = vietSystemFontBold.empty() ? vietSystemFont : vietSystemFontBold;
        io.Fonts->AddFontFromFileTTF(boldSrc.c_str(), 18.0f * scaleFactor, nullptr);
    } else {
        io.Fonts->AddFontFromMemoryTTF((void*)Geist_Bold, sizeof(Geist_Bold), 18.0f * scaleFactor, &mem_cfg);
        io.Fonts->AddFontFromMemoryTTF((void*)CJKNames_Ideographs, sizeof(CJKNames_Ideographs), 18.0f * scaleFactor, &cjk_name_cfg, cjk_ideograph_ranges);
        io.Fonts->AddFontFromMemoryTTF((void*)CJKNames_Korean, sizeof(CJKNames_Korean), 18.0f * scaleFactor, &cjk_name_cfg, cjk_hangul_ranges);
        if (!cjkSystemFont.empty())
            io.Fonts->AddFontFromFileTTF(cjkSystemFont.c_str(), 18.0f * scaleFactor, &cjk_name_cfg, cjk_full_ranges);
    }
    if (std::filesystem::exists(fontPath))
        io.Fonts->AddFontFromFileTTF(fontPath, 14.0f * scaleFactor, &cfg);
#endif

    /** Fonts[2] – VietName_Standalone: Arial covering every glyph in "Tieng Viet".
     *  Pushed via PushFont in the dropdown for the Vietnamese item so it renders
     *  from one typeface with no Geist mixing. */
    ImFontConfig viet_sa_cfg;
    viet_sa_cfg.FontDataOwnedByAtlas = false;
    io.Fonts->AddFontFromMemoryTTF((void*)VietName_Standalone, sizeof(VietName_Standalone),
                                   16.0f * scaleFactor, &viet_sa_cfg);

    /** Fonts[3] – Geist + VietName preview glyphs (ế, ệ) + CJK names.
     *  Added only when Vietnamese is active. The dropdown pushes this font around
     *  the entire combo so it looks identical to English mode. */
    if (isVietnamese) {
        io.Fonts->AddFontFromMemoryTTF((void*)GeistVariable, sizeof(GeistVariable),
                                       16.0f * scaleFactor, &mem_cfg);
        io.Fonts->AddFontFromMemoryTTF((void*)VietName_Standalone, sizeof(VietName_Standalone),
                                       16.0f * scaleFactor, &viet_preview_cfg, viet_preview_ranges);
        io.Fonts->AddFontFromMemoryTTF((void*)CJKNames_Ideographs, sizeof(CJKNames_Ideographs),
                                       16.0f * scaleFactor, &cjk_name_cfg, cjk_ideograph_ranges);
        io.Fonts->AddFontFromMemoryTTF((void*)CJKNames_Korean, sizeof(CJKNames_Korean),
                                       16.0f * scaleFactor, &cjk_name_cfg, cjk_hangul_ranges);
    }

    /** Explicitly set FreeType as the font loader to ensure color emoji support */
    io.Fonts->SetFontLoader(ImGuiFreeType::GetFontLoader());

    io.DisplayFramebufferScale = ImVec2(scaleFactor, scaleFactor);

    /** Reset current ImGui style */
    style = ImGuiStyle();
    style.ScaleAllSizes(scaleFactor);
    SetupColorScheme();
}

void SpawnRendererThread(GLFWwindow* window, const char* glsl_version, std::shared_ptr<RouterNav> router)
{
    glfwMakeContextCurrent(window);
#ifndef _WIN32
    glewExperimental = GL_TRUE;
#endif
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
#ifdef _WIN32
            Sleep((DWORD)(remainingTime * 1000));
#else
            std::this_thread::sleep_for(std::chrono::duration<double>(remainingTime));
#endif
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
