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

#ifndef SS_AMPLITUDE_AUDIO_OCCLUSION_PROCESSOR_H
#define SS_AMPLITUDE_AUDIO_OCCLUSION_PROCESSOR_H

#include <SparkyStudios/Audio/Amplitude/Amplitude.h>

#include <Core/EngineInternalState.h>
#include <Sound/Filters/BiquadResonantFilter.h>

namespace SparkyStudios::Audio::Amplitude
{
    static std::unordered_map<AmObjectID, FilterInstance*> gOcclusionFilters = {};

    [[maybe_unused]] static class OcclusionProcessor final : public SoundProcessor
    {
    public:
        OcclusionProcessor()
            : SoundProcessor("OcclusionProcessor")
            , _lpfCurve()
        {
            _lpfCurve.SetFader(Fader::ALGORITHM_EXPONENTIAL);
        }

        void Process(
            AmInt16Buffer out,
            AmInt16Buffer in,
            AmUInt64 frames,
            AmUInt64 bufferSize,
            AmUInt16 channels,
            AmUInt32 sampleRate,
            SoundInstance* sound) override
        {
            const float occlusion = sound->GetOcclusion();

            // Nothing to do if no occlusion
            if (occlusion == 0)
            {
                if (out != in)
                {
                    memcpy(out, in, bufferSize);
                }

                return;
            }

            _lpfCurve.SetStart({ 0, sampleRate / 2.0f });
            _lpfCurve.SetEnd({ 1, sampleRate / 2000.0f });

            const auto& lpfCurve = amEngine->GetState()->occlusion_config.lpf;
            const auto& gainCurve = amEngine->GetState()->occlusion_config.gain;

            if (const AmReal32 lpf = lpfCurve.Get(occlusion); lpf > 0)
            {
                if (gOcclusionFilters.find(sound->GetId()) == gOcclusionFilters.end())
                {
                    auto lpFilter = BiquadResonantFilter();
                    lpFilter.InitLowPass(std::ceil(_lpfCurve.Get(lpf)), 0.5f);
                    gOcclusionFilters[sound->GetId()] = lpFilter.CreateInstance();
                }

                // Update the filter coefficients
                gOcclusionFilters[sound->GetId()]->SetFilterParameter(BiquadResonantFilter::ATTRIBUTE_FREQUENCY, _lpfCurve.Get(lpf));

                // Apply Low Pass Filter
                gOcclusionFilters[sound->GetId()]->Process(out, frames, bufferSize, channels, sampleRate);
            }

            const AmReal32 gain = gainCurve.Get(occlusion);

            // Apply Gain
            for (AmUInt64 i = 0, l = frames * channels; i < l; i++)
            {
                out[i] = out[i] * AmFloatToFixedPoint(gain) >> kAmFixedPointBits;
                out[i] = AM_CLAMP(out[i], INT16_MIN, INT16_MAX);
            }
        }

        void ProcessInterleaved(
            AmInt16Buffer out,
            AmInt16Buffer in,
            AmUInt64 frames,
            AmUInt64 bufferSize,
            AmUInt16 channels,
            AmUInt32 sampleRate,
            SoundInstance* sound) override
        {
            const float occlusion = sound->GetOcclusion();

            // Nothing to do if no occlusion
            if (occlusion == 0)
            {
                if (out != in)
                {
                    memcpy(out, in, bufferSize);
                }

                return;
            }

            _lpfCurve.SetStart({ 0, sampleRate / 2.0f });
            _lpfCurve.SetEnd({ 1, sampleRate / 2000.0f });

            const auto& lpfCurve = amEngine->GetState()->occlusion_config.lpf;
            const auto& gainCurve = amEngine->GetState()->occlusion_config.gain;

            if (const AmReal32 lpf = lpfCurve.Get(occlusion); lpf > 0)
            {
                if (gOcclusionFilters.find(sound->GetId()) == gOcclusionFilters.end())
                {
                    auto lpFilter = BiquadResonantFilter();
                    lpFilter.InitLowPass(std::ceil(_lpfCurve.Get(lpf)), 0.5f);
                    gOcclusionFilters[sound->GetId()] = lpFilter.CreateInstance();
                }

                // Update the filter coefficients
                gOcclusionFilters[sound->GetId()]->SetFilterParameter(BiquadResonantFilter::ATTRIBUTE_FREQUENCY, _lpfCurve.Get(lpf));

                // Apply Low Pass Filter
                gOcclusionFilters[sound->GetId()]->ProcessInterleaved(out, frames, bufferSize, channels, sampleRate);
            }

            const AmReal32 gain = gainCurve.Get(occlusion);

            // Apply Gain
            for (AmUInt64 i = 0, l = frames * channels; i < l; i++)
            {
                out[i] = out[i] * AmFloatToFixedPoint(gain) >> kAmFixedPointBits;
                out[i] = AM_CLAMP(out[i], INT16_MIN, INT16_MAX);
            }
        }

        void Cleanup(SoundInstance* sound) override
        {
            if (gOcclusionFilters.find(sound->GetId()) == gOcclusionFilters.end())
                return;
            
            delete gOcclusionFilters[sound->GetId()];
            gOcclusionFilters.erase(sound->GetId());
        }

    private:
        CurvePart _lpfCurve;
    } gOcclusionProcessor; // NOLINT(cert-err58-cpp)
} // namespace SparkyStudios::Audio::Amplitude

#endif // SS_AMPLITUDE_AUDIO_OCCLUSION_PROCESSOR_H
