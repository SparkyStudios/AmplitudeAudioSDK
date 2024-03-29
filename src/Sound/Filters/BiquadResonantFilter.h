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

#ifndef SS_AMPLITUDE_AUDIO_BIQUAD_RESONANT_FILTER_H
#define SS_AMPLITUDE_AUDIO_BIQUAD_RESONANT_FILTER_H

#include <SparkyStudios/Audio/Amplitude/Sound/Filter.h>

namespace SparkyStudios::Audio::Amplitude
{
    class BiquadResonantFilter;

    struct BiquadResonantStateData
    {
        AmReal32 y1, y2, x1, x2;
    };

    class BiquadResonantFilterInstance : public FilterInstance
    {
    public:
        explicit BiquadResonantFilterInstance(BiquadResonantFilter* parent);
        ~BiquadResonantFilterInstance() override = default;

        void ProcessChannel(AmAudioSampleBuffer buffer, AmUInt16 channel, AmUInt64 frames, AmUInt16 channels, AmUInt32 sampleRate)
            override;

        AmAudioSample ProcessSample(AmAudioSample sample, AmUInt16 channel, AmUInt32 sampleRate) override;

    private:
        void ComputeBiquadResonantParams();

        BiquadResonantStateData _state[AM_MAX_CHANNELS];
        AmReal32 _a0, _a1, _a2, _b1, _b2;
        AmUInt32 _sampleRate;
    };

    [[maybe_unused]] static class BiquadResonantFilter final : public Filter
    {
        friend class BiquadResonantFilterInstance;

    public:
        enum ATTRIBUTE
        {
            ATTRIBUTE_WET = 0,
            ATTRIBUTE_TYPE,
            ATTRIBUTE_FREQUENCY,
            ATTRIBUTE_RESONANCE,
            ATTRIBUTE_GAIN,
            ATTRIBUTE_LAST
        };

        enum TYPE
        {
            TYPE_LOW_PASS = 0,
            TYPE_HIGH_PASS,
            TYPE_BAND_PASS,
            TYPE_PEAK,
            TYPE_NOTCH,
            TYPE_LOW_SHELF,
            TYPE_HIGH_SHELF,
            TYPE_LAST
        };

        BiquadResonantFilter();
        ~BiquadResonantFilter() override = default;

        AmResult Init(TYPE type, AmReal32 frequency, AmReal32 qOrS, AmReal32 gain);

        AmResult InitLowPass(AmReal32 frequency, AmReal32 q);

        AmResult InitHighPass(AmReal32 frequency, AmReal32 q);

        AmResult InitBandPass(AmReal32 frequency, AmReal32 q);

        AmResult InitPeaking(AmReal32 frequency, AmReal32 q, AmReal32 gain);

        AmResult InitNotching(AmReal32 frequency, AmReal32 q);

        AmResult InitLowShelf(AmReal32 frequency, AmReal32 s, AmReal32 gain);

        AmResult InitHighShelf(AmReal32 frequency, AmReal32 s, AmReal32 gain);

        [[nodiscard]] AmUInt32 GetParamCount() const override;

        [[nodiscard]] AmString GetParamName(AmUInt32 index) const override;

        [[nodiscard]] AmUInt32 GetParamType(AmUInt32 index) const override;

        [[nodiscard]] AmReal32 GetParamMax(AmUInt32 index) const override;

        [[nodiscard]] AmReal32 GetParamMin(AmUInt32 index) const override;

        FilterInstance* CreateInstance() override;

        void DestroyInstance(FilterInstance* instance) override;

    private:
        AmInt32 _filterType;
        AmReal32 _frequency;
        AmReal32 _resonance;
        AmReal32 _gain;
    } gBiquadResonantFilter; // NOLINT(cert-err58-cpp)
} // namespace SparkyStudios::Audio::Amplitude

#endif // SS_AMPLITUDE_AUDIO_BIQUAD_RESONANT_FILTER_H
