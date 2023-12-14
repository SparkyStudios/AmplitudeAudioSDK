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

class DummyPoolTask final : public Thread::PoolTask
{
public:
    void Work() override
    {
        _isExecuted = true;
    }

    [[nodiscard]] bool IsExecuted() const
    {
        return _isExecuted;
    }

private:
    bool _isExecuted = false;
};

TEST_CASE("Thread Mutex Tests", "[thread][amplitude]")
{
    WHEN("a thread mutex is created")
    {
        auto mutex = Thread::CreateMutex();

        THEN("it can be locked")
        {
            Thread::LockMutex(mutex);

            AND_THEN("it can be unlocked")
            {
                Thread::UnlockMutex(mutex);
            }
        }

        AND_THEN("it can be destroyed")
        {
            Thread::DestroyMutex(mutex);
        }
    }
}

TEST_CASE("Thread Pool Tests", "[thread][amplitude]")
{
    Thread::Pool pool;
    pool.Init(8);

    SECTION("initializes with the correct number of threads")
    {
        REQUIRE(pool.GetThreadCount() == 8);
    }

    WHEN("a task is added to the pool")
    {
        auto task = std::shared_ptr<DummyPoolTask>(amnew(DummyPoolTask), am_delete<MemoryPoolKind::Default, DummyPoolTask>{});
        pool.AddTask(task);

        THEN("the task is executed")
        {
            Thread::Sleep(10);
            REQUIRE(task->IsExecuted());
        }
    }
}