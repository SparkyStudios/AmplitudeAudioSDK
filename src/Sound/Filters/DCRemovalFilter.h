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

#ifndef SS_AMPLITUDE_AUDIO_DCREMOVALFILTER_H
#define SS_AMPLITUDE_AUDIO_DCREMOVALFILTER_H

#include <SparkyStudios/Audio/Amplitude/Sound/Filter.h>

namespace SparkyStudios::Audio::Amplitude
{
    class DCRemovalFilterInstance;

    class DCRemovalFilter : public Filter
    {
        friend class DCRemovalFilterInstance;

    public:
        enum ATTRIBUTE
        {
            ATTRIBUTE_WET = 0,
            ATTRIBUTE_LAST
        };

        DCRemovalFilter();

        AmResult Init(AmReal32 length = 0.1f);

        FilterInstance* CreateInstance() override;

    private:
        AmReal32 _length;
    };

    class DCRemovalFilterInstance : public FilterInstance
    {
    public:
        explicit DCRemovalFilterInstance(DCRemovalFilter* parent);
        ~DCRemovalFilterInstance() override;

        void Process(AmInt16Buffer buffer, AmUInt64 frames, AmUInt64 bufferSize, AmUInt16 channels, AmUInt32 sampleRate) override;
        void ProcessInterleaved(
            AmInt16Buffer buffer, AmUInt64 frames, AmUInt64 bufferSize, AmUInt16 channels, AmUInt32 sampleRate) override;

        AmInt16 ProcessSample(AmInt16 sample, AmUInt16 channel, AmUInt32 sampleRate) override;

    private:
        void InitBuffer(AmUInt16 channels, AmUInt32 sampleRate);

        AmInt32Buffer _buffer;
        AmInt32Buffer _totals;
        AmUInt64 _bufferLength;
        AmUInt64 _offset;
    };
} // namespace SparkyStudios::Audio::Amplitude

#endif // SS_AMPLITUDE_AUDIO_DCREMOVALFILTER_H