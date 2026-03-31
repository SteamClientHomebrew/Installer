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
#include <thread>
#include <functional>
#include <atomic>
#include <mutex>

class BackgroundWorker
{
  public:
    BackgroundWorker() = default;
    ~BackgroundWorker()
    {
        join();
    }

    BackgroundWorker(const BackgroundWorker&) = delete;
    BackgroundWorker& operator=(const BackgroundWorker&) = delete;

    void run(std::function<void()> fn)
    {
        join();
        m_thread = std::thread([this, fn = std::move(fn)]()
        {
            fn();
            m_busy.store(false, std::memory_order_release);
        });
        m_busy.store(true, std::memory_order_release);
    }

    bool busy() const
    {
        return m_busy.load(std::memory_order_acquire);
    }

    void join()
    {
        if (m_thread.joinable())
            m_thread.join();
    }

  private:
    std::thread m_thread;
    std::atomic<bool> m_busy{ false };
};

BackgroundWorker& GetWorker();
bool IsWorkerBusy();
void JoinWorker();
