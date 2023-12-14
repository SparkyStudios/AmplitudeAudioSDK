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

#include <SparkyStudios/Audio/Amplitude/Core/Memory.h>
#include <SparkyStudios/Audio/Amplitude/Core/Thread.h>

#if defined(AM_WINDOWS_VERSION)
#include <processthreadsapi.h>
#include <Windows.h>
#undef CreateMutex
#else
#include <ctime>
#include <pthread.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace SparkyStudios::Audio::Amplitude::Thread
{
    struct AmThreadData
    {
        AmThreadFunction mFunc;
        AmVoidPtr mParam;
    };

#if defined(AM_WINDOWS_VERSION)
    struct AmThreadHandleData
    {
        HANDLE thread;
        AmThreadData* data;
    };

    static DWORD WINAPI ThreadFunc(LPVOID d)
    {
        auto* p = static_cast<AmThreadData*>(d);
        p->mFunc(p->mParam);
        return 0;
    }

    AmMutexHandle CreateMutex(AmUInt64 spinCount)
    {
        auto* cs = ampoolnew(MemoryPoolKind::IO, CRITICAL_SECTION);
        AMPLITUDE_ASSERT(::InitializeCriticalSectionAndSpinCount(cs, spinCount) == TRUE);
        return static_cast<AmMutexHandle>(cs);
    }

    void DestroyMutex(AmMutexHandle handle)
    {
        if (handle == nullptr)
            return;

        auto* cs = static_cast<CRITICAL_SECTION*>(handle);
        ::DeleteCriticalSection(cs);
        ampooldelete(MemoryPoolKind::IO, CRITICAL_SECTION, cs);
    }

    void LockMutex(AmMutexHandle handle)
    {
        if (handle == nullptr)
            return;

        auto* cs = static_cast<CRITICAL_SECTION*>(handle);
        ::EnterCriticalSection(cs);
    }

    void UnlockMutex(AmMutexHandle handle)
    {
        if (handle == nullptr)
            return;

        auto* cs = static_cast<CRITICAL_SECTION*>(handle);
        ::LeaveCriticalSection(cs);
    }

    AmThreadHandle CreateThread(AmThreadFunction threadFunction, AmVoidPtr parameter)
    {
        auto* d = ampoolnew(MemoryPoolKind::IO, AmThreadData);
        d->mFunc = threadFunction;
        d->mParam = parameter;

        HANDLE h = ::CreateThread(nullptr, 0, ThreadFunc, (LPVOID)d, 0, nullptr);

        if (nullptr == h)
            return nullptr;

        auto* threadHandle = ampoolnew(MemoryPoolKind::IO, AmThreadHandleData);
        threadHandle->thread = h;
        threadHandle->data = d;

        return static_cast<AmThreadHandle>(threadHandle);
    }

    void Sleep(AmInt32 milliseconds)
    {
        ::Sleep(milliseconds);
    }

    void Wait(AmThreadHandle threadHandle)
    {
        auto* threadHandleData = static_cast<AmThreadHandleData*>(threadHandle);
        ::WaitForSingleObject(threadHandleData->thread, INFINITE);
    }

    void Release(AmThreadHandle threadHandle)
    {
        auto* threadHandleData = static_cast<AmThreadHandleData*>(threadHandle);
        ::CloseHandle(threadHandleData->thread);
        ampooldelete(MemoryPoolKind::IO, AmThreadData, threadHandleData->data);
        ampooldelete(MemoryPoolKind::IO, AmThreadHandleData, threadHandleData);
    }

    AmUInt64 GetTimeMillis()
    {
        return ::GetTickCount64();
    }

    AmThreadID GetCurrentThreadId()
    {
        return ::GetCurrentThreadId();
    }
#else // pthreads
    struct AmThreadHandleData
    {
        pthread_t thread;
        AmThreadData* data;
    };

    struct AmSpinLockData
    {
#if !defined(AM_NO_PTHREAD_SPINLOCK)
        pthread_spinlock_t lock;
#endif
        AmUInt64 count;
        pthread_mutex_t fallBackMutex;
        bool spinLocked;
    };

    static AmVoidPtr ThreadFunc(AmVoidPtr d)
    {
        auto* p = static_cast<AmThreadData*>(d);
        p->mFunc(p->mParam);
        return nullptr;
    }

    AmMutexHandle CreateMutex(AmUInt64 spinCount)
    {
        auto* lock = ampoolnew(MemoryPoolKind::IO, AmSpinLockData);
        lock->spinLocked = false;

#if !defined(AM_NO_PTHREAD_SPINLOCK)
        pthread_spin_init(&lock->lock, 0);
        lock->count = spinCount;
#endif

        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&lock->fallBackMutex, &attr);

        return static_cast<AmMutexHandle>(lock);
    }

    void DestroyMutex(AmMutexHandle handle)
    {
        if (handle == nullptr)
            return;

        auto* lock = static_cast<AmSpinLockData*>(handle);

#if !defined(AM_NO_PTHREAD_SPINLOCK)
        pthread_spin_destroy(&lock->lock);
#endif

        pthread_mutex_destroy(&lock->fallBackMutex);

        ampooldelete(MemoryPoolKind::IO, AmSpinLockData, lock);
    }

    void LockMutex(AmMutexHandle handle)
    {
        if (handle == nullptr)
            return;

        auto* lock = static_cast<AmSpinLockData*>(handle);

#if !defined(AM_NO_PTHREAD_SPINLOCK)
        AmUInt64 count = 0;
        while (count++ < lock->count)
        {
            if (pthread_spin_trylock(&lock->lock) == 0)
            {
                lock->spinLocked = true;
                return;
            }
        }
#endif

        if (!lock->spinLocked)
            pthread_mutex_lock(&lock->fallBackMutex);
    }

    void UnlockMutex(AmMutexHandle handle)
    {
        if (handle == nullptr)
            return;

        auto* lock = static_cast<AmSpinLockData*>(handle);

#if !defined(AM_NO_PTHREAD_SPINLOCK)
        if (lock->spinLocked)
            pthread_spin_unlock(&lock->lock);
        else
            pthread_mutex_unlock(&lock->fallBackMutex);
#else
        pthread_mutex_unlock(&lock->fallBackMutex);
#endif

        lock->spinLocked = false;
    }

    AmThreadHandle CreateThread(AmThreadFunction threadFunction, AmVoidPtr parameter)
    {
        auto* d = ampoolnew(MemoryPoolKind::IO, AmThreadData);
        d->mFunc = threadFunction;
        d->mParam = parameter;

        auto* threadHandle = ampoolnew(MemoryPoolKind::IO, AmThreadHandleData);
        threadHandle->data = d;

        pthread_create(&threadHandle->thread, nullptr, ThreadFunc, (AmVoidPtr)threadHandle->data);

        return threadHandle;
    }

    void Sleep(AmInt32 milliseconds)
    {
        // usleep(milliseconds * 1000);
        struct timespec req = { 0 };
        req.tv_sec = 0;
        req.tv_nsec = milliseconds * 1000000L;
        nanosleep(&req, nullptr);
    }

    void Wait(AmThreadHandle threadHandle)
    {
        const auto* threadHandleData = static_cast<AmThreadHandleData*>(threadHandle);
        pthread_join(threadHandleData->thread, nullptr);
    }

    void Release(AmThreadHandle threadHandle)
    {
        auto* threadHandleData = static_cast<AmThreadHandleData*>(threadHandle);
        pthread_detach(threadHandleData->thread);
        ampooldelete(MemoryPoolKind::IO, AmThreadData, threadHandleData->data);
        ampooldelete(MemoryPoolKind::IO, AmThreadHandleData, threadHandleData);
    }

    AmUInt64 GetTimeMillis()
    {
        struct timespec spec = { 0 };
        clock_gettime(CLOCK_REALTIME, &spec);
        return spec.tv_sec * 1000 + (spec.tv_nsec / 1.0e6);
    }

    AmThreadID GetCurrentThreadId()
    {
#if defined(AM_APPLE_VERSION)
        AmThreadID tid = 0;
        pthread_threadid_np(pthread_self(), &tid);
        return tid;
#else
        pid_t tid = syscall(__NR_gettid);
        return (AmThreadID)tid;
#endif
    }
#endif

    static void PoolWorker(AmVoidPtr param)
    {
        auto* pPool = static_cast<Pool*>(param);

        while (pPool->IsRunning())
        {
            if (std::shared_ptr<PoolTask> t = pPool->GetWork(); t == nullptr)
            {
                Sleep(1);
            }
            else
            {
                t->Work();
            }
        }
    }

    bool PoolTask::Ready()
    {
        return true;
    }

    AwaitablePoolTask::AwaitablePoolTask()
        : _condition()
        , _mutex()
    {}

    void AwaitablePoolTask::Work()
    {
        AwaitableWork();
        _condition.notify_all();
    }

    void AwaitablePoolTask::Await()
    {
        std::unique_lock lock(_mutex);
        _condition.wait(lock);
    }

    bool AwaitablePoolTask::Await(AmUInt64 duration)
    {
        std::unique_lock lock(_mutex);
        return _condition.wait_for(lock, std::chrono::milliseconds(duration)) == std::cv_status::no_timeout;
    }

    Pool::Pool()
        : _threadCount(0)
        , _thread(nullptr)
        , _workMutex(nullptr)
        , _taskCount(0)
        , _robin(0)
        , _running(false)
    {
        for (auto& i : _taskArray)
            i = nullptr;
    }

    Pool::~Pool()
    {
        _running = false;

        for (AmUInt32 i = 0; i < _threadCount; i++)
        {
            Wait(_thread[i]);
            Release(_thread[i]);
        }

        ampoolfree(MemoryPoolKind::IO, _thread);

        if (_workMutex)
            DestroyMutex(_workMutex);
    }

    void Pool::Init(AmUInt32 threadCount)
    {
        if (_running || threadCount == 0)
            return;

        _taskCount = 0;
        _workMutex = CreateMutex();
        _running = true;
        _threadCount = threadCount;
        _thread = static_cast<AmThreadHandle*>(ampoolmalloc(MemoryPoolKind::IO, sizeof(void*) * threadCount));

        for (AmUInt32 i = 0; i < _threadCount; i++)
            _thread[i] = CreateThread(PoolWorker, this);
    }

    void Pool::AddTask(std::shared_ptr<PoolTask> task)
    {
        if (_threadCount == 0)
        {
            if (task->Ready())
                task->Work();
        }
        else
        {
            if (_workMutex)
                LockMutex(_workMutex);

            if (_taskCount == AM_MAX_THREAD_POOL_TASKS)
            {
                // If we're at max tasks, do the task on calling thread
                // (we're in trouble anyway, might as well slow down adding more tasks)
                if (_workMutex)
                    UnlockMutex(_workMutex);

                if (task->Ready())
                    task->Work();
            }
            else
            {
                _taskArray[_taskCount] = task;
                _taskCount++;

                if (_workMutex)
                    UnlockMutex(_workMutex);
            }
        }
    }

    std::shared_ptr<PoolTask> Pool::GetWork()
    {
        std::shared_ptr<PoolTask> t = nullptr;

        if (_workMutex)
            LockMutex(_workMutex);

        if (_taskCount > 0)
        {
            AmInt32 r;
            AmInt32 c = 0;

            do
            {
                r = _robin % _taskCount;
                _robin++;
                t = _taskArray[r];

                if (t->Ready())
                {
                    _taskArray[r] = _taskArray[_taskCount - 1];
                    _taskCount--;

                    break;
                }
                else
                {
                    t = nullptr;
                }

                c++;
            } while (c < _taskCount);
        }

        if (_workMutex)
            UnlockMutex(_workMutex);

        return t;
    }

    AmUInt32 Pool::GetThreadCount() const
    {
        return _threadCount;
    }

    bool Pool::IsRunning() const
    {
        return _running;
    }

    bool Pool::HasTasks() const
    {
        return _taskCount > 0;
    }

    AmInt32 Pool::GetTaskCount() const
    {
        return _taskCount;
    }
} // namespace SparkyStudios::Audio::Amplitude::Thread
