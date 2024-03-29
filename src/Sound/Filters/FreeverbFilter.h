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

#ifndef SS_AMPLITUDE_AUDIO_FREEVERBFILTER_H
#define SS_AMPLITUDE_AUDIO_FREEVERBFILTER_H

#include <SparkyStudios/Audio/Amplitude/Sound/Filter.h>

#include <Utils/Freeverb/ReverbModel.h>

namespace SparkyStudios::Audio::Amplitude
{
    class FreeverbFilter;

    class FreeverbFilterInstance : public FilterInstance
    {
    public:
        explicit FreeverbFilterInstance(FreeverbFilter* parent);
        ~FreeverbFilterInstance() override;

        void Process(
            AmAudioSampleBuffer buffer, AmUInt64 frames, AmUInt64 bufferSize, AmUInt16 channels, AmUInt32 sampleRate) override;

    private:
        Freeverb::ReverbModel* _model;
    };

    [[maybe_unused]] static class FreeverbFilter final : public Filter
    {
        friend class FreeverbFilterInstance;

    public:
        enum ATTRIBUTE
        {
            ATTRIBUTE_WET = 0,
            ATTRIBUTE_DRY,
            ATTRIBUTE_ROOM_SIZE,
            ATTRIBUTE_DAMP,
            ATTRIBUTE_WIDTH,
            ATTRIBUTE_MODE,
            ATTRIBUTE_LAST
        };

        FreeverbFilter();
        ~FreeverbFilter() override = default;

        AmResult Init(AmReal32 roomSize, AmReal32 damp, AmReal32 width, AmReal32 mode);

        [[nodiscard]] AmUInt32 GetParamCount() const override;

        [[nodiscard]] AmString GetParamName(AmUInt32 index) const override;

        [[nodiscard]] AmUInt32 GetParamType(AmUInt32 index) const override;

        [[nodiscard]] AmReal32 GetParamMax(AmUInt32 index) const override;

        [[nodiscard]] AmReal32 GetParamMin(AmUInt32 index) const override;

        FilterInstance* CreateInstance() override;

        void DestroyInstance(FilterInstance* instance) override;

    private:
        AmReal32 _roomSize;
        AmReal32 _damp;
        AmReal32 _width;
        AmReal32 _mode;
    } gFreeverbFilter; // NOLINT(cert-err58-cpp)
} // namespace SparkyStudios::Audio::Amplitude

#endif // SS_AMPLITUDE_AUDIO_FREEVERBFILTER_H
