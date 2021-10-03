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

#include <Sound/Filters/BiquadResonantFilter.h>

namespace SparkyStudios::Audio::Amplitude
{
    static constexpr AmInt32 kAmBiquadFixedPointShift = 14;

    static AmInt32 AmBiquadFloatToFP(const float x)
    {
        return static_cast<AmInt32>(x * (1 << kAmBiquadFixedPointShift));
    }

    BiquadResonantFilter::BiquadResonantFilter()
        : _filterType(TYPE_LOW_PASS)
        , _frequency(1000.0f)
        , _resonance(0.707107f)
        , _gain(1.0f)
    {}

    AmResult BiquadResonantFilter::Init(TYPE type, float frequency, float resonance, float gain)
    {
        if (type < 0 || type >= TYPE_LAST || frequency <= 0 || resonance <= 0 || gain <= 0)
            return AM_ERROR_INVALID_PARAMETER;

        _filterType = type;
        _frequency = frequency;
        _resonance = resonance;
        _gain = gain;

        return AM_ERROR_NO_ERROR;
    }

    AmUInt32 BiquadResonantFilter::GetParamCount()
    {
        return ATTRIBUTE_LAST;
    }

    AmString BiquadResonantFilter::GetParamName(AmUInt32 index)
    {
        if (index >= ATTRIBUTE_LAST)
            return nullptr;

        AmString names[ATTRIBUTE_LAST] = { "Wet", "Type", "Frequency", "Latency" };

        return names[index];
    }

    AmUInt32 BiquadResonantFilter::GetParamType(AmUInt32 index)
    {
        if (index == ATTRIBUTE_TYPE)
            return PARAM_INT;

        return PARAM_FLOAT;
    }

    float BiquadResonantFilter::GetParamMax(AmUInt32 index)
    {
        switch (index)
        {
        case ATTRIBUTE_WET:
            return 1;
        case ATTRIBUTE_TYPE:
            return TYPE_LAST - 1;
        case ATTRIBUTE_FREQUENCY:
            return 8000.0f;
        case ATTRIBUTE_RESONANCE:
            return 20.0f;
        case ATTRIBUTE_GAIN:
            return 1.0f;
        default:
            return 1.0f;
        }
    }

    float BiquadResonantFilter::GetParamMin(AmUInt32 index)
    {
        switch (index)
        {
        case ATTRIBUTE_FREQUENCY:
            return 10.0f;
        case ATTRIBUTE_RESONANCE:
            return 0.707107f;
        default:
            return 0.0f;
        }
    }

    FilterInstance* BiquadResonantFilter::CreateInstance()
    {
        return new BiquadResonantFilterInstance(this);
    }

    BiquadResonantFilterInstance::BiquadResonantFilterInstance(BiquadResonantFilter* parent)
        : FilterInstance(parent)
        , _state{}
        , _a0(0)
        , _a1(0)
        , _a2(0)
        , _b1(0)
        , _b2(0)
        , _isDirty(false)
    {
        for (auto&& i : _state)
        {
            i.x1 = 0;
            i.y1 = 0;
            i.x2 = 0;
            i.y2 = 0;
        }

        Init(5);

        m_parameters[BiquadResonantFilter::ATTRIBUTE_GAIN] = parent->_gain;
        m_parameters[BiquadResonantFilter::ATTRIBUTE_RESONANCE] = parent->_resonance;
        m_parameters[BiquadResonantFilter::ATTRIBUTE_FREQUENCY] = parent->_frequency;
        m_parameters[BiquadResonantFilter::ATTRIBUTE_TYPE] = static_cast<float>(parent->_filterType);

        _sampleRate = 44100;

        ComputeBiquadResonantParams();
    }

    void BiquadResonantFilterInstance::Process(
        AmInt16Buffer buffer, AmUInt64 samples, AmUInt64 bufferSize, AmUInt16 channels, AmUInt32 sampleRate)
    {
        if (buffer == nullptr)
            return;

        AmInt16Buffer pY = buffer;

        for (AmUInt32 n = 0; n < samples; n += 1)
        {
            ProcessFrame(pY, channels, sampleRate);
            pY += channels;
        }
    }

    void BiquadResonantFilterInstance::ProcessFrame(AmInt16Buffer frame, AmUInt16 channels, AmUInt32 sampleRate)
    {
        if (m_numParamsChanged &
                (1 << BiquadResonantFilter::ATTRIBUTE_FREQUENCY | 1 << BiquadResonantFilter::ATTRIBUTE_RESONANCE |
                 1 << BiquadResonantFilter::ATTRIBUTE_GAIN | 1 << BiquadResonantFilter::ATTRIBUTE_TYPE) ||
            sampleRate != _sampleRate)
        {
            _sampleRate = sampleRate;
            ComputeBiquadResonantParams();
        }

        m_numParamsChanged = 0;

        for (AmUInt32 c = 0; c < channels; c++)
        {
            const AmInt32 x = frame[c];
            /* */ AmInt32 y;

            const AmInt32 x1 = _state[c].x1;
            const AmInt32 x2 = _state[c].x2;
            const AmInt32 y1 = _state[c].y1;
            const AmInt32 y2 = _state[c].y2;

            y = _a0 * x + _a1 * x1 + _a2 * x2 - _b1 * y1 - _b2 * y2 >> kAmBiquadFixedPointShift;
            y = x + ((y - x) * AmBiquadFloatToFP(m_parameters[BiquadResonantFilter::ATTRIBUTE_WET]) >> kAmBiquadFixedPointShift);

            frame[c] = static_cast<AmInt16>(AM_CLAMP(y, INT16_MIN, INT16_MAX));

            _state[c].x1 = x;
            _state[c].x2 = x1;
            _state[c].y1 = y;
            _state[c].y2 = y1;
        }
    }

    void BiquadResonantFilterInstance::ComputeBiquadResonantParams()
    {
        _isDirty = false;

        const float q = m_parameters[BiquadResonantFilter::ATTRIBUTE_RESONANCE];
        const float omega = 2.0f * M_PI * m_parameters[BiquadResonantFilter::ATTRIBUTE_FREQUENCY] / static_cast<float>(_sampleRate);
        const float sinOmega = sinf(omega);
        const float cosOmega = cosf(omega);
        const float alpha = sinOmega / (2.0f * q);
        const float mu = std::powf(10, (m_parameters[BiquadResonantFilter::ATTRIBUTE_GAIN] / 40));

        float scalar;

        switch (static_cast<AmUInt32>(m_parameters[BiquadResonantFilter::ATTRIBUTE_TYPE]))
        {
        default:
        case BiquadResonantFilter::TYPE_LOW_PASS:
            scalar = 1.0f / (1.0f + alpha);
            _a0 = AmBiquadFloatToFP(0.5f * (1.0f - cosOmega) * scalar);
            _a1 = AmBiquadFloatToFP((1.0f - cosOmega) * scalar);
            _a2 = _a0;
            _b1 = AmBiquadFloatToFP(-2.0f * cosOmega * scalar);
            _b2 = AmBiquadFloatToFP((1.0f - alpha) * scalar);
            break;
        case BiquadResonantFilter::TYPE_HIGH_PASS:
            scalar = 1.0f / (1.0f + alpha);
            _a0 = AmBiquadFloatToFP(0.5f * (1.0f + cosOmega) * scalar);
            _a1 = AmBiquadFloatToFP(-(1.0f + cosOmega) * scalar);
            _a2 = _a0;
            _b1 = AmBiquadFloatToFP(-2.0f * cosOmega * scalar);
            _b2 = AmBiquadFloatToFP((1.0f - alpha) * scalar);
            break;
        case BiquadResonantFilter::TYPE_BAND_PASS:
            scalar = 1.0f / (1.0f + alpha);
            _a0 = AmBiquadFloatToFP(q * alpha * scalar);
            _a1 = AmBiquadFloatToFP(0);
            _a2 = -_a0;
            _b1 = AmBiquadFloatToFP(-2.0f * cosOmega * scalar);
            _b2 = AmBiquadFloatToFP((1.0f - alpha) * scalar);
            break;
        case BiquadResonantFilter::TYPE_PEAK:
            scalar = 1.0f / (1.0f + (alpha / mu));
            _a0 = AmBiquadFloatToFP((1.0f + (alpha * mu)) * scalar);
            _a1 = AmBiquadFloatToFP(-2.0f * cosOmega * scalar);
            _a2 = AmBiquadFloatToFP((1.0f - (alpha * mu)) * scalar);
            _b1 = AmBiquadFloatToFP(-2.0f * cosOmega * scalar);
            _b2 = AmBiquadFloatToFP((1.0f - (alpha / mu)) * scalar);
            break;
        }
    }
} // namespace SparkyStudios::Audio::Amplitude