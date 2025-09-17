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
#include <string>
#include <router.h>
#include <memory>
#include <dpi.h>

// Easing function: ease-in-out quadratic
float EaseInOut(float t);
float EaseInOutTime(float t, float b, float c, float d);

struct AnimationState
{
    float currentPosY;
    float startPosY;
    float elapsedTime;
    bool isAnimating;
    bool wasHovered;

    float lastXDPI = XDPI, lastYDPI = YDPI;
};

float SmoothFloat(const std::string& id, float targetOffset, bool currentState, float displacement, float duration,
                  std::tuple<float, float> colorTransition = std::make_tuple(0.f, 0.f));

float EaseInOutFloat(const std::string& id, float lowerBound, float upperBound, bool currentState, float duration);

float EaseOutBack(float t, float b, float c, float d, float overshoot = 1.20158f);