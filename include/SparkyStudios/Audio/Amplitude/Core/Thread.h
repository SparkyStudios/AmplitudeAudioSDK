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

#pragma once

#ifndef SS_AMPLITUDE_AUDIO_THREAD_H
#define SS_AMPLITUDE_AUDIO_THREAD_H

#include <condition_variable>

#include <SparkyStudios/Audio/Amplitude/Core/Common.h>

namespace SparkyStudios::Audio::Amplitude
{
#if defined(AM_WINDOWS_VERSION)
#undef CreateMutex
#endif

    /**
     * @brief The AmThreadFunction signature is used to create threads.
     */
    typedef void (*AmThreadFunction)(AmVoidPtr param);

    typedef AmVoidPtr AmThreadHandle;
    typedef AmVoidPtr AmMutexHandle;
    typedef AmUInt64 AmThreadID;

    namespace Thread
    {
        /**
         * @brief Creates a mutex object.
         *
         * A mutex is an object that a thread can acquire, preventing other
         * threads from acquiring it.
         *
         * To acquire the mutex ownership, you should use LockMutex() with
         * the mutex handle as parameter. To release the ownership, use UnlockMutex()
         * with the mutex handle as parameter.
         */
        AM_API_PUBLIC AmMutexHandle CreateMutex(AmUInt64 spinCount = 100);

        /**
         * @brief Destroys a mutex object.
         *
         * @param handle The mutex object handle.
         */
        AM_API_PUBLIC void DestroyMutex(AmMutexHandle handle);

        /**
         * @brief Takes ownership of a mutex.
         *
         * @param handle The mutex object handle.
         */
        AM_API_PUBLIC void LockMutex(AmMutexHandle handle);

        /**
         * @brief Releases ownership of a mutex.
         *
         * @param handle The mutex object handle.
         */
        AM_API_PUBLIC void UnlockMutex(AmMutexHandle handle);

        /**
         * @brief Creates a new thread.
         *
         * @param threadFunction The function to run in the thread.
         * @param parameter An optional shared data to pass to the thread
         */
        AM_API_PUBLIC AmThreadHandle CreateThread(AmThreadFunction threadFunction, AmVoidPtr parameter = nullptr);

        /**
         * @brief Makes the calling thread sleep for the given amount of milliseconds.
         *
         * @param milliseconds The amount of time the calling thread should sleep.
         */
        AM_API_PUBLIC void Sleep(AmInt32 milliseconds);

        /**
         * @brief Waits for the given thread to stop.
         *
         * @param thread The handle of the thread to wait.
         */
        AM_API_PUBLIC void Wait(AmThreadHandle thread);

        /**
         * @brief Manually stops a thread execution.
         *
         * @param thread The handle of the thread to wait.
         */
        AM_API_PUBLIC void Release(AmThreadHandle thread);

        /**
         * @brief Gets the total execution time in milliseconds for the calling thread.
         */
        AM_API_PUBLIC AmUInt64 GetTimeMillis();

        /**
         * @brief Gets the handle of the calling thread.
         */
        AM_API_PUBLIC AmThreadID GetCurrentThreadId();

        /**
         * @brief Base class for pool tasks.
         */
        class AM_API_PUBLIC PoolTask
        {
        public:
            virtual ~PoolTask() = default;

            /**
             * @brief Main pool task execution function.
             *
             * When this task will be picked by the pool scheduler, this method
             * will be called to execute the task.
             */
            virtual void Work() = 0;

            /**
             * @brief Checks if the task is ready to be picked by the pool scheduler.
             *
             * @return @c true if the task is ready @c false otherwise.
             */
            virtual bool Ready();
        };

        /**
         * @brief A pool task that allows the a thread to wait until it finishes.
         */
        class AM_API_PUBLIC AwaitablePoolTask : public PoolTask
        {
        public:
            AwaitablePoolTask();
            ~AwaitablePoolTask() override = default;

            void Work() override;

            /**
             * @bbrief Pool task execution function.
             */
            virtual void AwaitableWork() = 0;

            /**
             * @brief Makes the calling thread wait for this task to finish.
             */
            void Await();

            /**
             * @brief Makes the calling thread wait for this task to finish.
             *
             * @param duration The maximum amount of time to wait in milliseconds.
             */
            bool Await(AmUInt64 duration);

        private:
            std::condition_variable _condition;
            std::mutex _mutex;
        };

        /**
         * @brief Pool tasks scheduler class.
         *
         * The Pool tasks scheduler can pick and run pool tasks on several multiple
         * threads. The number of threads is defined at initialization.
         *
         * The maximum number of tasks the pool can manage is defined by the @c AM_MAX_THREAD_POOL_TASKS
         * macro. The default value is 1024.
         */
        class AM_API_PUBLIC Pool
        {
        public:
            /**
             * @brief Creates a new pool tasks scheduler instance.
             */
            Pool();

            // Waits for the threads to finish. Work may be unfinished.
            ~Pool();

            /**
             * @brief Initialize and run thread pool.
             *
             * @param threadCount The number of thread in the pool. For thread count 0, work is done
             * at AddTask() call in the calling thread.
             */
            void Init(AmUInt32 threadCount);

            /**
             * @brief Add a task to the tasks list.
             *
             * @param task The PoolTask to add. The task may not be automatically deleted when the work is done.
             */
            void AddTask(std::shared_ptr<PoolTask> task);

            /**
             * @brief Called from worker thread to get a new task.
             *
             * @note This method is called internally, and should not be called in user code.
             *
             * @return The next PoolTask to execute, or nullptr if no task is available.
             */
            std::shared_ptr<PoolTask> GetWork();

            /**
             * @brief Gets the number of threads this pool is using.
             */
            [[nodiscard]] AmUInt32 GetThreadCount() const;

            /**
             * @brief Indicates that the pool is running.
             */
            [[nodiscard]] bool IsRunning() const;

            /**
             * @brief Indicates that the pool has tasks pending.
             */
            [[nodiscard]] bool HasTasks() const;

            /**
             * @brief Gets the number of tasks in the pool.
             *
             * @return The number of tasks in the pool.
             */
            [[nodiscard]] AmInt32 GetTaskCount() const;

        private:
            AmUInt32 _threadCount; // number of threads
            AmThreadHandle* _thread; // array of thread handles
            AmMutexHandle _workMutex; // mutex to protect task array/max task
            std::shared_ptr<PoolTask> _taskArray[AM_MAX_THREAD_POOL_TASKS]{}; // pointers to tasks
            AmInt32 _taskCount; // how many tasks are pending
            AmInt32 _robin; // cyclic counter, used to pick jobs for threads
            volatile bool _running; // running flag, used to flag threads to Stop
        };
    } // namespace Thread
} // namespace SparkyStudios::Audio::Amplitude

#endif // SS_AMPLITUDE_AUDIO_THREAD_H
