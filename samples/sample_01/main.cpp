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

#include <iostream>

#include <SparkyStudios/Audio/Amplitude/Amplitude.h>

using namespace SparkyStudios::Audio::Amplitude;

static constexpr AmUInt32 kAppModeMainMenu = 0;
static constexpr AmUInt32 kAppModeCollectionTest = 1;
static constexpr AmUInt32 kAppModeSwitchContainerTest = 2;

struct ExecutionContext
{
    AmUInt32 appMode = kAppModeMainMenu;

    Channel mainMenuBackgroundChannel;
    Channel collectionSampleChannel;
    Channel switchContainerChannel;

    AmUInt32 currentSwitchState = 3;

    bool printMemoryStats = false;
    bool pause = false;
    bool stop = false;
};

static void log(const char* fmt, va_list args)
{
#if defined(AM_WCHAR_SUPPORTED)
    vfwprintf(stdout, AM_STRING_TO_OS_STRING(fmt), args);
#else
    vfprintf(stdout, fmt, args);
#endif
}

static void printMemoryStats()
{
    std::map<MemoryPoolKind, std::string> map = {
        { MemoryPoolKind::Amplimix, "Amplimix" },   { MemoryPoolKind::Codec, "Codec" },         { MemoryPoolKind::Engine, "Engine" },
        { MemoryPoolKind::Filtering, "Filtering" }, { MemoryPoolKind::SoundData, "SoundData" }, { MemoryPoolKind::Default, "Default" },
    };

    for (auto&& kind : map)
    {
        const auto& stats = amMemory->GetStats(kind.first);

        std::cout << "Pool Name - " << kind.second << std::endl;
        std::cout << "    Allocations Count: " << stats.allocCount << std::endl;
        std::cout << "    Frees Count: " << stats.freeCount << std::endl;
        std::cout << "    Total Memory used: " << stats.maxMemoryUsed << std::endl;
        std::cout << std::endl;
    }
}

static void run(AmVoidPtr param)
{
    MemoryManager::Initialize(MemoryManagerConfig());

    FileLoader loader = FileLoader();
    loader.SetBasePath(AM_OS_STRING("../assets"));

    amEngine->SetFileLoader(loader);

    // Initialize Amplitude.
    if (!amEngine->Initialize(AM_OS_STRING("audio_config.amconfig")))
        return;

    if (!amEngine->LoadSoundBank(AM_OS_STRING("sample_01.ambank")))
        return;

    // Wait for the sound files to complete loading.
    amEngine->StartLoadingSoundFiles();
    while (!amEngine->TryFinalizeLoadingSoundFiles())
        Thread::Sleep(1);

    // Cache the master bus, so we can adjust the gain.
    auto masterBus = amEngine->FindBus("master");

    // Cache the background sound handle for the main menu.
    auto mainMenuBackgroundHandle = amEngine->GetSoundHandle("symphony");

    // Cache the CollectionHandle for the collection sample.
    auto collectionHandle = amEngine->GetCollectionHandle("throw_collection_1");
    if (collectionHandle == nullptr)
    {
        fprintf(stderr, "Could not find collection handle %s\n", "throw_collection_1");
        return;
    }

    // Cache the SwitchContainerHandle for the switch sample.
    auto footstepsHandle = amEngine->GetSwitchContainerHandle(200);
    if (footstepsHandle == nullptr)
    {
        fprintf(stderr, "Could not find switch container handle %s\n", "footsteps");
        return;
    }

    // Cache the EventHandle for the event sample.
    auto playEventHandle = amEngine->GetEventHandle("play_throw");
    if (playEventHandle == nullptr)
    {
        fprintf(stderr, "Could not find event handle %s\n", "play_throw");
        return;
    }

    auto stopEventHandle = amEngine->GetEventHandle("stop_throw");
    if (stopEventHandle == nullptr)
    {
        fprintf(stderr, "Could not find event handle %s\n", "stop_throw");
        return;
    }

    // Setup the default listener
    auto listener = amEngine->AddListener(1);
    listener.SetLocation(AM_V3(0, 0, 0));
    listener.SetOrientation(AM_V3(0, 0, 1), AM_V3(0, 1.0f, 0));

    // Setup a virtual player entity, for the footsteps.
    auto player = amEngine->AddEntity(1);
    player.SetLocation(AM_V3(0, 0, 0));
    player.SetOrientation(AM_V3(0, 0, 1), AM_V3(0, 1.0f, 0));

    amEngine->SetDefaultListener(&listener);

    AmUInt32 lastSwitch = 0;

    while (true)
    {
        auto* ctx = static_cast<ExecutionContext*>(param);

        if (ctx->stop)
            break;

        if (ctx->printMemoryStats)
        {
            printMemoryStats();
            ctx->printMemoryStats = false;
        }

        amEngine->Pause(ctx->pause);

        if (ctx->pause)
            continue;

        if (ctx->appMode == kAppModeMainMenu)
        {
            if (ctx->collectionSampleChannel.Valid() && ctx->collectionSampleChannel.Playing())
                ctx->collectionSampleChannel.Stop(kAmSecond);

            if (ctx->switchContainerChannel.Valid() && ctx->switchContainerChannel.Playing())
                ctx->switchContainerChannel.Stop(kAmSecond);

            if (!ctx->mainMenuBackgroundChannel.Valid() ||
                ctx->mainMenuBackgroundChannel.GetPlaybackState() == ChannelPlaybackState::Stopped)
                ctx->mainMenuBackgroundChannel = amEngine->Play(mainMenuBackgroundHandle);
            else if (ctx->mainMenuBackgroundChannel.GetPlaybackState() == ChannelPlaybackState::Paused)
                ctx->mainMenuBackgroundChannel.Resume(kAmSecond);
        }
        else
        {
            if (ctx->mainMenuBackgroundChannel.Valid())
                ctx->mainMenuBackgroundChannel.Pause(kAmSecond);

            if (ctx->appMode == kAppModeCollectionTest)
            {
                if (!ctx->collectionSampleChannel.Valid() || !ctx->collectionSampleChannel.Playing())
                    ctx->collectionSampleChannel = amEngine->Play(collectionHandle);
            }
            else if (ctx->appMode == kAppModeSwitchContainerTest)
            {
                if (ctx->currentSwitchState == 3 && lastSwitch != ctx->currentSwitchState)
                {
                    CallLogFunc("Walking on metal\n");
                    amEngine->SetSwitchState("surface_type", "metal");
                }

                if (ctx->currentSwitchState == 4 && lastSwitch != ctx->currentSwitchState)
                {
                    CallLogFunc("Walking on grass\n");
                    amEngine->SetSwitchState("surface_type", "grass");
                }

                if (ctx->currentSwitchState == 5 && lastSwitch != ctx->currentSwitchState)
                {
                    CallLogFunc("Walking on snow\n");
                    amEngine->SetSwitchState("surface_type", "snow");
                }

                if (!ctx->switchContainerChannel.Valid() || !ctx->switchContainerChannel.Playing())
                    ctx->switchContainerChannel = amEngine->Play(footstepsHandle, player);

                lastSwitch = ctx->currentSwitchState;
            }
        }

        amEngine->AdvanceFrame(1.0 / 60);

        Thread::Sleep(static_cast<AmInt32>(kAmSecond / 60.0));
    }

    amEngine->Deinitialize();
}

int main(int argc, char* argv[])
{
    ExecutionContext ctx{};
    auto t = Thread::CreateThread(run, &ctx);

    std::cout << "Amplitude Audio SDK Sample 01." << std::endl;

    while (true)
    {
        if (ctx.appMode == kAppModeMainMenu)
        {
            std::cout << std::endl;
            std::cout << "Select a sample:" << std::endl;
            std::cout << kAppModeCollectionTest << " - Collection Sample" << std::endl;
            std::cout << kAppModeSwitchContainerTest << " - Switch Container Sample" << std::endl;
            std::cout << "Type 0 to quit the program, and 9 to print memory stats." << std::endl << std::endl;
            std::cout << "Enter a value: ";
        }

        if (ctx.appMode == kAppModeCollectionTest)
        {
            std::cout << std::endl;
            std::cout << "Playing sounds from collection \"throw_collection_1\"." << std::endl;
            std::cout << "All sounds are played one by one, and when the end of the collection is reached, all sounds are played in the "
                         "reverse order."
                      << std::endl;
            std::cout << "This behavior has been configured in the collection file. You can play with the settings, recompile the project "
                         "using flatbuffers, and preview it here."
                      << std::endl
                      << std::endl;
            std::cout << "Press 0 to go back to the main menu, and 9 to print memory stats: ";
        }

        if (ctx.appMode == kAppModeSwitchContainerTest)
        {
            std::cout << std::endl;
            std::cout << "Playing sounds from switch container \"footsteps\"." << std::endl;
            std::cout << "The sounds are played using a virtual entity, and changes according to the state of the \"surface_type\" switch."
                      << std::endl;
            std::cout << "To change the current state of the switch, choose between the following values:" << std::endl;
            std::cout << "3 - Metal" << (ctx.currentSwitchState == 3 ? " (active)" : "") << std::endl;
            std::cout << "4 - Grass" << (ctx.currentSwitchState == 4 ? " (active)" : "") << std::endl;
            std::cout << "5 - Snow" << (ctx.currentSwitchState == 5 ? " (active)" : "") << std::endl << std::endl;
            std::cout << "Press 0 to go back to the main menu, and 9 to print memory stats: ";
        }

        int input = 0;
        std::cin >> input;

        if (input == 9)
        {
            std::cout << std::endl;
            printMemoryStats();
            continue;
        }

        if (ctx.appMode == kAppModeMainMenu)
        {
            if (input != 0 && input != kAppModeCollectionTest && input != kAppModeSwitchContainerTest)
            {
                std::cout << "The input is invalid." << std::endl;
                continue;
            }

            if (input == 0)
            {
                ctx.stop = true;
                break;
            }

            ctx.appMode = input;
        }
        else if (ctx.appMode == kAppModeCollectionTest)
        {
            if (input == 0)
            {
                ctx.appMode = kAppModeMainMenu;
                continue;
            }

            std::cout << "The input is invalid." << std::endl;
        }
        else if (ctx.appMode == kAppModeSwitchContainerTest)
        {
            if (input == 0)
            {
                ctx.appMode = kAppModeMainMenu;
                continue;
            }

            if (input == 3 || input == 4 || input == 5)
            {
                ctx.currentSwitchState = input;
                continue;
            }

            std::cout << "The input is invalid." << std::endl;
        }
    }

    Thread::Wait(t);

    printMemoryStats();
}