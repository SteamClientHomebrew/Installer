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
#include <vector>
#include <functional>
#include <string>
#include <memory>

class TaskScheduler
{
  public:
    struct TaskResult
    {
        bool success;
        std::string message;
    };

    using Task = std::function<TaskResult(std::unique_ptr<double>&)>;

    TaskScheduler();
    void addTask(Task task);
    void addTasks(const std::vector<Task>& newTasks);
    double getProgress() const;
    void run();
    bool hasFailed() const;
    std::string getFailureReason() const;
    size_t getTaskCount() const;
    size_t getCurrentTaskIndex() const;

  private:
    bool m_hasAnyTaskFailed;
    std::string m_failureReason;

    std::vector<Task> tasks;
    double overallProgress;
    size_t currentTaskIndex;
    std::unique_ptr<double> currentTaskProgress;
};
