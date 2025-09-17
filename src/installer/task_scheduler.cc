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

#include "task_scheduler.h"
#include <algorithm>

TaskScheduler::TaskScheduler() : overallProgress(0.0), currentTaskIndex(0), currentTaskProgress(std::make_unique<double>(0.0))
{
}

void TaskScheduler::addTask(Task task)
{
    tasks.push_back(task);
}

void TaskScheduler::addTasks(const std::vector<Task>& newTasks)
{
    tasks.insert(tasks.end(), newTasks.begin(), newTasks.end());
}

double TaskScheduler::getProgress() const
{
    if (tasks.empty()) {
        return 1.0;
    }

    double completedTasksProgress = static_cast<double>(currentTaskIndex) / tasks.size();
    double currentTaskContribution = 0.0;

    if (currentTaskIndex < tasks.size() && currentTaskProgress) {
        currentTaskContribution = (*currentTaskProgress) / tasks.size();
    }

    return completedTasksProgress + currentTaskContribution;
}

void TaskScheduler::run()
{
    if (tasks.empty()) {
        return;
    }

    for (currentTaskIndex = 0; currentTaskIndex < tasks.size(); currentTaskIndex++) {
        currentTaskProgress = std::make_unique<double>(0.0);
        tasks[currentTaskIndex](currentTaskProgress);
        *currentTaskProgress = std::min(1.0, *currentTaskProgress);
        overallProgress = getProgress();
    }
    overallProgress = 1.0;
}

size_t TaskScheduler::getTaskCount() const
{
    return tasks.size();
}

size_t TaskScheduler::getCurrentTaskIndex() const
{
    return currentTaskIndex;
}