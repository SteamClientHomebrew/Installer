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

#include <animate.h>
#include <cmath>
#include <iostream>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <imgui.h>
#include <memory>
#include <dpi.h>
#include <mutex>

/** Based on https://stackoverflow.com/questions/13462001/ease-in-and-ease-out-animation-formula */
float EaseInOut(float t)
{
    return t < 0.5f ? 2 * t * t : -1 + (4 - 2 * t) * t;
}

float EaseInOutFloat(const std::string& id, float lowerBound, float upperBound, bool currentState, float duration)
{
    struct FloatAnimationState
    {
        bool wasHovered = false;
        bool isAnimating = false;
        bool isReversing = false;

        float elapsedTime = 0.0f;
        float currentValue = 0.0f;
        float minValue = 0.0f;
        float maxValue = 1.0f;

        float lastXDPI = XDPI, lastYDPI = YDPI;

        FloatAnimationState() = default;
        FloatAnimationState(float lower, float upper)
            : wasHovered(false), isAnimating(false), isReversing(false), elapsedTime(0.0f), currentValue(lower), minValue(lower), maxValue(upper)
        {
        }
    };

    static std::unordered_map<std::string, FloatAnimationState> animations;
    static std::mutex animationsMutex;

    std::lock_guard<std::mutex> lock(animationsMutex);

    auto it = animations.find(id);
    if (it == animations.end()) {
        animations[id] = FloatAnimationState(lowerBound, upperBound);
        animations[id].currentValue = currentState ? upperBound : lowerBound;
        animations[id].wasHovered = currentState;
    }

    FloatAnimationState& state = animations[id];

    float deltaTime = ImGui::GetIO().DeltaTime;

    if (currentState != state.wasHovered) {
        state.wasHovered = currentState;
        state.isAnimating = true;
        state.isReversing = !currentState;
        state.elapsedTime = 0.0f;
    }

    if (state.isAnimating) {
        state.elapsedTime += deltaTime;
        float progress = state.elapsedTime / duration;

        if (progress >= 1.0f) {
            progress = 1.0f;
            state.isAnimating = false;
        }

        float easedProgress = EaseInOut(progress);

        float startValue = state.currentValue;
        float targetValue = state.isReversing ? state.minValue : state.maxValue;

        state.currentValue = startValue + (targetValue - startValue) * easedProgress;
    }

    return state.currentValue;
}

/**
 * Smoothly float an element to a target offset.
 * @param id The unique identifier for the animation.
 * @param targetOffset The target offset.
 * @param currentState The current state.
 * @param displacement How high the element should float, relative to 0.
 * @param duration The duration of the animation.
 *
 *
 */
float SmoothFloat(const std::string& id, float targetOffset, bool currentState, float displacement, float duration, std::tuple<float, float> colorTransition)
{
    static std::unordered_map<std::string, AnimationState> animations;
    static std::mutex animationsMutex;

    std::lock_guard<std::mutex> lock(animationsMutex);

    const auto [lowerBound, upperBound] = colorTransition;
    AnimationState& state = animations[id];

    float currentXDPI = XDPI;
    float currentYDPI = YDPI;

    if (currentXDPI != state.lastXDPI || currentYDPI != state.lastYDPI) {
        state.currentPosY = ImGui::GetCursorPosY();
        state.lastXDPI = currentXDPI;
        state.lastYDPI = currentYDPI;
    }

    if (state.currentPosY == 0.0f)
        state.currentPosY = ImGui::GetCursorPosY();

    float deltaTime = ImGui::GetIO().DeltaTime;

    if (currentState != state.wasHovered) {
        state.wasHovered = currentState;
        state.isAnimating = true;
        state.elapsedTime = 0.0f;
        state.startPosY = state.currentPosY;
    }

    float targetPosY = currentState ? targetOffset - displacement : targetOffset;
    float result = upperBound;

    if (state.isAnimating) {
        state.elapsedTime += deltaTime;

        float progress = state.elapsedTime / duration;
        if (progress >= 1.0f) {
            progress = 1.0f;
            state.isAnimating = false;
        }

        float easedProgress = EaseInOut(progress);
        result = lowerBound + (upperBound - lowerBound) * easedProgress;
        state.currentPosY = state.startPosY + (easedProgress * (targetPosY - state.startPosY));
    }

    ImGui::SetCursorPosY(state.currentPosY);
    return result;
}

float EaseInOutTime(float t, float b, float c, float d)
{
    t /= d / 2;
    if (t < 1)
        return c / 2 * t * t + b;
    t--;
    return -c / 2 * (t * (t - 2) - 1) + b;
}

/** Based on https://stackoverflow.com/questions/5699091/how-to-implement-easeoutback-easing-in-css-transform-animation */
float EaseOutBack(float t, float b, float c, float d, float overshoot)
{
    t = t / d - 1;
    return c * (t * t * ((overshoot + 1) * t + overshoot) + 1) + b;
}
