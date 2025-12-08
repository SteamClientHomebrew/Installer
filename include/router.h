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
#include <functional>
#include <vector>
#include <memory>
#include <iostream>

class RouterNav;
using Component = std::function<void(std::shared_ptr<RouterNav> router, float xOffset)>;

enum AnimationDirection
{
    FORWARD = 1,
    BACKWARD = -1
};

class RouterNav
{
  public:
    RouterNav(const std::vector<Component>& components) : softCanGoBack(false), softCanGoForward(false), components(components), currentIndex(0), isAnimating(false), animTime(0.0f)
    {
    }

    void navigateNext();
    void navigateBack();

    bool canGoBack() const
    {
        return softCanGoBack && currentIndex > 0;
    }
    bool canGoForward() const
    {
        return softCanGoForward && currentIndex + 1 < components.size();
    }

    void setCanGoBack(const bool newValue);
    void setCanGoForward(const bool newValue);

    Component getCurrentComponent() const;
    Component getTransitioningComponent() const;

    void update();

    float getCurrentOffset(float viewportWidth) const;
    float getTransitioningOffset(float viewportWidth) const;

    void setComponents(const std::vector<Component>& components)
    {
        this->components = components;
    }

  private:
    std::vector<Component> components;

    size_t currentIndex;
    size_t targetIndex;

    bool isAnimating;
    float animTime;
    int animDirection;
    const float animationDuration = 0.3f;

    bool softCanGoBack = false;
    bool softCanGoForward = false;

    float lerp(float a, float b, float t) const;
    void startAnimation(int direction);
};
