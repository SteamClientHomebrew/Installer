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

#include <renderer.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <components.h>
#include <wndproc.h>

using namespace ImGui;

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

        // if (IsKeyPressed(ImGuiKey_MouseX1)) {
        //     if (router->canGoBack())
        //         router->navigateBack();
        // }
        // if (IsKeyPressed(ImGuiKey_MouseX2)) {
        //     if (router->canGoForward())
        //         router->navigateNext();
        // }
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