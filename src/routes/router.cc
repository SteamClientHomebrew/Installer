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

#include <vector>
#include <algorithm>
#include <imgui.h>
#include <router.h>
#include <math.h>

float easeInOut(float t)
{
    return t < 0.5f ? 4.0f * t * t * t : 1.0f - pow(-2.0f * t + 2.0f, 3) / 2.0f;
}

void RouterNav::navigateNext()
{
    if (currentIndex + 1 < components.size() && !isAnimating) {
        startAnimation(AnimationDirection::FORWARD);
        this->setCanGoBack(true);
    }
}

void RouterNav::navigateBack()
{
    if (currentIndex > 0 && !isAnimating) {
        startAnimation(AnimationDirection::BACKWARD);
        this->setCanGoForward(true);
    }
}

void RouterNav::setCanGoBack(const bool newValue)
{
    softCanGoBack = newValue;
}

void RouterNav::setCanGoForward(const bool newValue)
{
    softCanGoForward = newValue;
}

Component RouterNav::getCurrentComponent() const
{
    return components[currentIndex];
}

Component RouterNav::getTransitioningComponent() const
{
    return isAnimating ? components[targetIndex] : nullptr;
}

void RouterNav::update()
{
    float deltaTime = ImGui::GetIO().DeltaTime;
    if (isAnimating) {
        animTime += deltaTime / animationDuration;
        if (animTime >= 1.0f) {
            animTime = 1.0f;
            isAnimating = false;
            currentIndex = targetIndex;
        }
    }
}

float RouterNav::getCurrentOffset(float viewportWidth) const
{
    return isAnimating ? lerp(0.0f, -viewportWidth * animDirection, easeInOut(animTime)) : 0.0f;
}

float RouterNav::getTransitioningOffset(float viewportWidth) const
{
    return isAnimating ? lerp(viewportWidth * animDirection, 0.0f, easeInOut(animTime)) : 0.0f;
}

float RouterNav::lerp(float a, float b, float t) const
{
    return a + (b - a) * t;
}

void RouterNav::startAnimation(int direction)
{
    isAnimating = true;
    animTime = 0.0f;
    animDirection = direction;
    targetIndex = currentIndex + direction;
}
