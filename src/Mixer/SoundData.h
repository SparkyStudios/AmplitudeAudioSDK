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

#ifndef SS_AMPLITUDE_AUDIO_SOUNDDATA_H
#define SS_AMPLITUDE_AUDIO_SOUNDDATA_H

#include <SparkyStudios/Audio/Amplitude/Core/Common.h>

namespace SparkyStudios::Audio::Amplitude
{
    struct SoundChunk
    {
        AmInt32 length;
        AmInt32 frames;

        AudioBuffer buffer;

        static SoundChunk* CreateChunk(AmInt32 frames, AmInt32 channels);
        static void DestroyChunk(SoundChunk* chunk);
    };

    struct SoundData
    {
        SoundChunk* chunk;
        AmInt32 length;
        AmVoidPtr userData;
        SoundFormat format;
        bool stream;

        static SoundData* CreateMusic(const SoundFormat& format, SoundChunk* chunk, AmVoidPtr userData);
        static SoundData* CreateSound(const SoundFormat& format, SoundChunk* chunk, AmVoidPtr userData);

        void Destroy();
    };
} // namespace SparkyStudios::Audio::Amplitude

#endif // SS_AMPLITUDE_AUDIO_SOUNDDATA_H