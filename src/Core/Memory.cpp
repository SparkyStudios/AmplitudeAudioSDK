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

#include <set>

#include <SparkyStudios/Audio/Amplitude/Core/Memory.h>

#include <Utils/SmMalloc/smmalloc.h>

namespace SparkyStudios::Audio::Amplitude
{
    static MemoryManager* gMemManager = nullptr;

    static sm_allocator gMemAllocator = nullptr;

    static std::map<MemoryPoolKind, std::set<AmVoidPtr>> gMemPoolsData = {};

#if !defined(AM_NO_MEMORY_STATS)
    static std::map<MemoryPoolKind, MemoryPoolStats> gMemPoolsStats = {};
#endif

    MemoryManagerConfig::MemoryManagerConfig()
        : malloc(nullptr)
        , realloc(nullptr)
        , alignedMalloc(nullptr)
        , alignedRealloc(nullptr)
        , free(nullptr)
        , totalReservedMemorySize(nullptr)
        , sizeOf(nullptr)
        , bucketsCount(4)
        , bucketsSizeInBytes(16 * 1024 * 1024)
    {}

#if !defined(AM_NO_MEMORY_STATS)
    MemoryPoolStats::MemoryPoolStats(MemoryPoolKind kind)
        : pool(kind)
    {
        maxMemoryUsed.store(0);
        allocCount.store(0);
        freeCount.store(0);
    }

    MemoryPoolStats& MemoryPoolStats::operator=(const MemoryPoolStats& other)
    {
        pool = other.pool;
        maxMemoryUsed.store(other.maxMemoryUsed.load());
        allocCount.store(other.allocCount.load());
        freeCount.store(other.freeCount.load());

        return *this;
    }
#endif

    void MemoryManager::Initialize(const MemoryManagerConfig& config)
    {
        if (gMemManager == nullptr)
        {
            gMemManager = new MemoryManager(config);
        }

#if !defined(AM_NO_MEMORY_STATS)
        gMemPoolsStats[MemoryPoolKind::Amplimix] = MemoryPoolStats(MemoryPoolKind::Amplimix);
        gMemPoolsStats[MemoryPoolKind::Codec] = MemoryPoolStats(MemoryPoolKind::Codec);
        gMemPoolsStats[MemoryPoolKind::Engine] = MemoryPoolStats(MemoryPoolKind::Engine);
        gMemPoolsStats[MemoryPoolKind::Filtering] = MemoryPoolStats(MemoryPoolKind::Filtering);
        gMemPoolsStats[MemoryPoolKind::SoundData] = MemoryPoolStats(MemoryPoolKind::SoundData);
#endif
    }

    void MemoryManager::Deinitialize()
    {
        if (gMemManager != nullptr)
        {
            delete gMemManager;
            gMemManager = nullptr;
        }
    }

    bool MemoryManager::IsInitialized()
    {
        return gMemManager != nullptr;
    }

    MemoryManager* MemoryManager::GetInstance()
    {
        AMPLITUDE_ASSERT(gMemManager != nullptr);
        return gMemManager;
    }

    MemoryManager::MemoryManager(MemoryManagerConfig config)
        : _config(config)
    {
        if (config.malloc == nullptr)
        {
            gMemAllocator = _sm_allocator_create(config.bucketsCount, config.bucketsSizeInBytes);
        }
        else
        {
            AMPLITUDE_ASSERT(
                config.realloc != nullptr && config.free != nullptr && config.alignedMalloc != nullptr &&
                config.alignedRealloc != nullptr && config.sizeOf != nullptr);
            gMemAllocator = nullptr;
        }
    }

    MemoryManager::~MemoryManager()
    {
        if (gMemAllocator != nullptr)
        {
            _sm_allocator_destroy(gMemAllocator);
        }
    }

    AmVoidPtr MemoryManager::Malloc(MemoryPoolKind pool, AmSize size) const
    {
#if !defined(AM_NO_MEMORY_STATS)
        gMemPoolsStats[pool].maxMemoryUsed.fetch_add(size, std::memory_order_relaxed);
        gMemPoolsStats[pool].allocCount.fetch_add(1, std::memory_order_relaxed);
#endif

        if (_config.malloc != nullptr)
        {
            return _config.malloc(pool, size);
        }
        else
        {
            AmVoidPtr p = _sm_malloc(gMemAllocator, size, 0);
            gMemPoolsData[pool].insert(p);
            return p;
        }
    }

    AmVoidPtr MemoryManager::Malign(MemoryPoolKind pool, AmSize size, AmUInt32 alignment) const
    {
#if !defined(AM_NO_MEMORY_STATS)
        gMemPoolsStats[pool].maxMemoryUsed.fetch_add(size, std::memory_order_relaxed);
        gMemPoolsStats[pool].allocCount.fetch_add(1, std::memory_order_relaxed);
#endif

        if (_config.alignedMalloc != nullptr)
        {
            return _config.alignedMalloc(pool, size, alignment);
        }
        else
        {
            AmVoidPtr p = _sm_malloc(gMemAllocator, size, alignment);
            gMemPoolsData[pool].insert(p);
            return p;
        }
    }

    AmVoidPtr MemoryManager::Realloc(MemoryPoolKind pool, AmVoidPtr address, AmSize size) const
    {
        if (address == nullptr)
        {
#if !defined(AM_NO_MEMORY_STATS)
            gMemPoolsStats[pool].maxMemoryUsed.fetch_add(size, std::memory_order_relaxed);
            gMemPoolsStats[pool].allocCount.fetch_add(1, std::memory_order_relaxed);
#endif
        }

        if (_config.realloc != nullptr)
        {
            return _config.realloc(pool, address, size);
        }
        else
        {
            AmVoidPtr p = _sm_realloc(gMemAllocator, address, size, 0);
            if (auto it = gMemPoolsData[pool].find(address); it != gMemPoolsData[pool].end())
            {
                gMemPoolsData[pool].erase(it);
            }
            gMemPoolsData[pool].insert(p);
            return p;
        }
    }

    AmVoidPtr MemoryManager::Realign(MemoryPoolKind pool, AmVoidPtr address, AmSize size, AmUInt32 alignment) const
    {
        if (address == nullptr)
        {
#if !defined(AM_NO_MEMORY_STATS)
            gMemPoolsStats[pool].maxMemoryUsed.fetch_add(size, std::memory_order_relaxed);
            gMemPoolsStats[pool].allocCount.fetch_add(1, std::memory_order_relaxed);
#endif
        }

        if (_config.alignedRealloc != nullptr)
        {
            return _config.alignedRealloc(pool, address, size, alignment);
        }
        else
        {
            AmVoidPtr p = _sm_realloc(gMemAllocator, address, size, alignment);
            if (auto it = gMemPoolsData[pool].find(address); it != gMemPoolsData[pool].end())
            {
                gMemPoolsData[pool].erase(it);
            }
            gMemPoolsData[pool].insert(p);
            return p;
        }
    }

    void MemoryManager::Free(MemoryPoolKind pool, AmVoidPtr address) const
    {
#if !defined(AM_NO_MEMORY_STATS)
        gMemPoolsStats[pool].freeCount.fetch_add(1, std::memory_order_relaxed);
#endif

        if (_config.free != nullptr)
        {
            _config.free(pool, address);
        }
        else
        {
            _sm_free(gMemAllocator, address);
            if (auto it = gMemPoolsData[pool].find(address); it != gMemPoolsData[pool].end())
            {
                gMemPoolsData[pool].erase(it);
            }
        }
    }

    AmSize MemoryManager::TotalReservedMemorySize() const
    {
        if (_config.totalReservedMemorySize != nullptr)
        {
            return _config.totalReservedMemorySize();
        }
        else
        {
            AmSize total = 0;
            for (auto&& pool : gMemPoolsData)
            {
                for (auto&& p : pool.second)
                {
                    total += _sm_msize(gMemAllocator, p);
                }
            }

            return total;
        }
    }

    AmSize MemoryManager::SizeOf(MemoryPoolKind pool, AmVoidPtr address) const
    {
        if (_config.sizeOf != nullptr)
        {
            return _config.sizeOf(pool, address);
        }
        else
        {
            return _sm_msize(gMemAllocator, address);
        }
    }

#if !defined(AM_NO_MEMORY_STATS)
    const MemoryPoolStats& MemoryManager::GetStats(MemoryPoolKind pool) const
    {
        return gMemPoolsStats[pool];
    }
#endif
} // namespace SparkyStudios::Audio::Amplitude
