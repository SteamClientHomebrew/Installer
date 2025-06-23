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

#define GLFW_EXPOSE_NATIVE_WIN32
#include <wndproc.h>
#include <iostream>
#include <renderer.h>
#include <dpi.h>
#include <SDL_syswm.h>
#include <SDL.h>

WNDPROC g_OriginalWindProcCallback;
bool    isTitleBarHovered = false;
SDL_Window* window;
std::shared_ptr<RouterNav> g_routerPtr;

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_NCCALCSIZE:
        {
            // if (wParam && lParam)
            // {
            //     NCCALCSIZE_PARAMS* pParams = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
            //     pParams->rgrc[0].top += 1;
            //     pParams->rgrc[0].right -= 1;
            //     pParams->rgrc[0].bottom -= 1;
            //     pParams->rgrc[0].left += 1;
            // }
            return 0;
        }
        case WM_NCHITTEST:
        {
            POINT cursorPos;
            GetCursorPos(&cursorPos);
            ScreenToClient(hWnd, &cursorPos);

            RECT clientRect;
            GetClientRect(hWnd, &clientRect);

            const int titleBarHeight   = ScaleY(100);
            const int closeButtonWidth = ScaleX(70);
            const int backButtonWidth  = ScaleX(70);

            bool inTitleBar        = (cursorPos.y >= 0 && cursorPos.y <= titleBarHeight);
            bool inCloseButtonArea = (cursorPos.x >= (clientRect.right - closeButtonWidth) && cursorPos.x <= clientRect.right);
            bool inBackButtonArea  = (g_routerPtr->canGoBack() && cursorPos.x >= 0 && cursorPos.x <= backButtonWidth);

            return inTitleBar && !inCloseButtonArea && !inBackButtonArea ? HTCAPTION : HTCLIENT;
        }
        case WM_NCACTIVATE:
        {
            return true;
        }
        case WM_DESTROY:
        {
            timeEndPeriod(1);
        }
    }
    
    return CallWindowProc(g_OriginalWindProcCallback, hWnd, uMsg, wParam, lParam);
}

void SetBorderlessWindowStyle(SDL_Window* window, std::shared_ptr<RouterNav> router)
{
    ::window = window;
    ::g_routerPtr = router;

    HWND hWnd;

    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    
    if (SDL_GetWindowWMInfo(window, &wmInfo)) {
        if (wmInfo.subsystem == SDL_SYSWM_WINDOWS) {
            hWnd = wmInfo.info.win.window; 
        }
    }

    // Get current window position and size
    RECT windowRect;
    GetWindowRect(hWnd, &windowRect);
    int width  = windowRect.right - windowRect.left;
    int height = windowRect.bottom - windowRect.top;
    int x = windowRect.left;
    int y = windowRect.top;

    // Store original window proc before making changes
    g_OriginalWindProcCallback = (WNDPROC)GetWindowLongPtr(hWnd, GWLP_WNDPROC);
    
    // Remove caption bar and borders by modifying window style
    LONG_PTR style = GetWindowLongPtr(hWnd, GWL_STYLE);
    style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
    SetWindowLongPtr(hWnd, GWL_STYLE, style);
    
    // Set custom window proc
    SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProc));
    
    // Apply the style changes without using SWP_FRAMECHANGED
    SetWindowPos(hWnd, NULL, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
}