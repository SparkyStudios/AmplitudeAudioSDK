// Copyright (c) 2021-present Sparky Studios. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <catch2/catch_test_macros.hpp>

#include <SparkyStudios/Audio/Amplitude/Amplitude.h>

using namespace SparkyStudios::Audio::Amplitude;

class DummyPoolTask : public Thread::PoolTask
{
public:
    void Work() override
    {
        _isExecuted = true;
        _executingThreadId = Thread::GetCurrentThreadId();
    }

    [[nodiscard]] bool IsExecuted() const
    {
        return _isExecuted;
    }

    [[nodiscard]] AmThreadID GetExecutingThreadId() const
    {
        return _executingThreadId;
    }

private:
    bool _isExecuted = false;
    AmThreadID _executingThreadId = 0;
};

class AwaitableDummyPoolTask final : public Thread::AwaitablePoolTask
{
public:
    void AwaitableWork() override
    {
        Thread::Sleep(500);
        _isExecuted = true;
    }

    [[nodiscard]] bool IsExecuted() const
    {
        return _isExecuted;
    }

private:
    bool _isExecuted = false;
};

class NeverReadyPoolTask final : public DummyPoolTask
{
public:
    bool Ready() override
    {
        return false;
    }
};

TEST_CASE("Thread Tests", "[thread][amplitude]")
{
    WHEN("a thread mutex is created")
    {
        const auto mutex = Thread::CreateMutex();

        THEN("it can be locked")
        {
            Thread::LockMutex(mutex);

            AND_THEN("it can be unlocked")
            {
                Thread::UnlockMutex(mutex);
            }
        }

        THEN("it can be destroyed")
        {
            Thread::DestroyMutex(mutex);
        }
    }

    SECTION("can correctly sleep a thread for a duration")
    {
        const auto start = Thread::GetTimeMillis();
        Thread::Sleep(100);
        const auto end = Thread::GetTimeMillis();

        REQUIRE((end - start - 100) < 10); // Offering a 10ms window
    }
}

TEST_CASE("Thread Pool Tests", "[thread][amplitude]")
{
    WHEN("a thread pool is created with threads")
    {
        Thread::Pool pool;
        pool.Init(8);

        THEN("it initializes with the correct number of threads")
        {
            REQUIRE(pool.GetThreadCount() == 8);
        }

        THEN("it cannot initialize twice")
        {
            pool.Init(4);
            REQUIRE_FALSE(pool.GetThreadCount() == 4);
            REQUIRE(pool.GetThreadCount() == 8);
        }

        WHEN("a ready task is added to the pool")
        {
            std::shared_ptr<DummyPoolTask> task(amnew(DummyPoolTask), am_delete<MemoryPoolKind::Default, DummyPoolTask>{});
            REQUIRE_FALSE(task->IsExecuted());

            pool.AddTask(task);

            THEN("the task is executed")
            {
                Thread::Sleep(10); // Wait for the task to execute
                REQUIRE(task->IsExecuted());

                AND_THEN("the task is removed from the pool")
                {
                    REQUIRE_FALSE(pool.HasTasks());
                }
            }
        }

        WHEN("more than the maximum number of supported tasks is added to the pool")
        {
            for (size_t i = 0; i < AM_MAX_THREAD_POOL_TASKS + 100; i++)
            {
                auto task = std::make_shared<DummyPoolTask>();
                REQUIRE_FALSE(task->IsExecuted());

                bool willExecuteWorkInCallerThread = pool.GetTaskCount() >= AM_MAX_THREAD_POOL_TASKS;

                pool.AddTask(task);

                if (willExecuteWorkInCallerThread)
                {
                    REQUIRE(task->IsExecuted());
                    REQUIRE(task->GetExecutingThreadId() == Thread::GetCurrentThreadId());
                }
            }
        }

        WHEN("a not ready task is added to the pool")
        {
            std::shared_ptr<NeverReadyPoolTask> task(amnew(NeverReadyPoolTask), am_delete<MemoryPoolKind::Default, NeverReadyPoolTask>{});
            REQUIRE_FALSE(task->IsExecuted());

            pool.AddTask(task);

            THEN("the task is not executed")
            {
                Thread::Sleep(10); // Wait for the task to execute
                REQUIRE_FALSE(task->IsExecuted());

                AND_THEN("the task is not removed from the pool")
                {
                    REQUIRE(pool.HasTasks());
                }
            }

            AND_THEN("the pool still have the task")
            {
                REQUIRE(pool.HasTasks());
                REQUIRE(pool.GetTaskCount() == 1);
            }
        }

        WHEN("an awaitable ready task is added to the pool")
        {
            std::shared_ptr<AwaitableDummyPoolTask> task(
                amnew(AwaitableDummyPoolTask), am_delete<MemoryPoolKind::Default, AwaitableDummyPoolTask>{});
            REQUIRE_FALSE(task->IsExecuted());

            pool.AddTask(task);

            THEN("the task is not executed before the wait time")
            {
                REQUIRE_FALSE(task->Await(10));
                REQUIRE_FALSE(task->IsExecuted());
            }

            THEN("the task is executed after the wait time")
            {
                task->Await();
                REQUIRE(task->IsExecuted());
            }
        }
    }

    WHEN("a thread pool is created whithout threads")
    {
        Thread::Pool pool2;
        pool2.Init(0);

        THEN("it is not running")
        {
            REQUIRE_FALSE(pool2.IsRunning());
        }

        AND_THEN("it execute tasks in the caller thread")
        {
            auto task = std::shared_ptr<DummyPoolTask>(amnew(DummyPoolTask), am_delete<MemoryPoolKind::Default, DummyPoolTask>{});
            REQUIRE_FALSE(task->IsExecuted());

            pool2.AddTask(task);
            REQUIRE(task->IsExecuted());
            REQUIRE(task->GetExecutingThreadId() == Thread::GetCurrentThreadId());
        }
    }
}
