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
#include <Utils/Utils.h>

namespace SparkyStudios::Audio::Amplitude
{
    BiquadResonantFilter::BiquadResonantFilter()
        : _filterType(TYPE_LOW_PASS)
        , _frequency(1000.0f)
        , _resonance(0.707107f)
        , _gain(0.0f)
    {}

    AmResult BiquadResonantFilter::Init(TYPE type, AmReal32 frequency, AmReal32 resonance, AmReal32 gain)
    {
        if (type < 0 || type >= TYPE_LAST || frequency <= 0 || resonance <= 0)
            return AM_ERROR_INVALID_PARAMETER;

        _filterType = type;
        _frequency = frequency;
        _resonance = resonance;
        _gain = gain;

        return AM_ERROR_NO_ERROR;
    }

    AmResult BiquadResonantFilter::InitLowPass(AmReal32 frequency, AmReal32 q)
    {
        return Init(TYPE_LOW_PASS, frequency, q, 0.0f);
    }

    AmResult BiquadResonantFilter::InitHighPass(AmReal32 frequency, AmReal32 q)
    {
        return Init(TYPE_HIGH_PASS, frequency, q, 0.0f);
    }

    AmResult BiquadResonantFilter::InitBandPass(AmReal32 frequency, AmReal32 q)
    {
        return Init(TYPE_BAND_PASS, frequency, q, 0.0f);
    }

    AmResult BiquadResonantFilter::InitPeaking(AmReal32 frequency, AmReal32 q, AmReal32 gain)
    {
        return Init(TYPE_PEAK, frequency, q, gain);
    }

    AmResult BiquadResonantFilter::InitNotching(AmReal32 frequency, AmReal32 q)
    {
        return Init(TYPE_NOTCH, frequency, q, 0.0);
    }

    AmResult BiquadResonantFilter::InitLowShelf(AmReal32 frequency, AmReal32 s, AmReal32 gain)
    {
        return Init(TYPE_LOW_SHELF, frequency, s, gain);
    }

    AmResult BiquadResonantFilter::InitHighShelf(AmReal32 frequency, AmReal32 s, AmReal32 gain)
    {
        return Init(TYPE_HIGH_SHELF, frequency, s, gain);
    }

    AmUInt32 BiquadResonantFilter::GetParamCount()
    {
        return ATTRIBUTE_LAST;
    }

    AmString BiquadResonantFilter::GetParamName(AmUInt32 index)
    {
        if (index >= ATTRIBUTE_LAST)
            return "";

        // clang-format off
        AmString names[ATTRIBUTE_LAST] = {
            "Wet", "Type", "Frequency",
            _filterType == TYPE_LOW_SHELF || _filterType == TYPE_HIGH_SHELF ? "S" : "Q",
            "Gain"
        };
        // clang-format on

        return names[index];
    }

    AmUInt32 BiquadResonantFilter::GetParamType(AmUInt32 index)
    {
        if (index == ATTRIBUTE_TYPE)
            return PARAM_INT;

        return PARAM_FLOAT;
    }

    AmReal32 BiquadResonantFilter::GetParamMax(AmUInt32 index)
    {
        switch (index)
        {
        case ATTRIBUTE_WET:
            return 1;
        case ATTRIBUTE_TYPE:
            return TYPE_LAST - 1;
        case ATTRIBUTE_FREQUENCY:
            return 30000.0f;
        case ATTRIBUTE_RESONANCE:
            return 40.0f;
        case ATTRIBUTE_GAIN:
            return 30.0f;
        default:
            return 1.0f;
        }
    }

    AmReal32 BiquadResonantFilter::GetParamMin(AmUInt32 index)
    {
        switch (index)
        {
        case ATTRIBUTE_FREQUENCY:
            return 10.0f;
        case ATTRIBUTE_RESONANCE:
            return 0.025f;
        case ATTRIBUTE_GAIN:
            return -30.0f;
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

        Init(BiquadResonantFilter::ATTRIBUTE_LAST);

        m_parameters[BiquadResonantFilter::ATTRIBUTE_GAIN] = parent->_gain;
        m_parameters[BiquadResonantFilter::ATTRIBUTE_RESONANCE] = parent->_resonance;
        m_parameters[BiquadResonantFilter::ATTRIBUTE_FREQUENCY] = parent->_frequency;
        m_parameters[BiquadResonantFilter::ATTRIBUTE_TYPE] = static_cast<AmReal32>(parent->_filterType);

        _sampleRate = 44100;

        ComputeBiquadResonantParams();
    }

    void BiquadResonantFilterInstance::ProcessChannel(
        AmInt16Buffer buffer, AmUInt16 channel, AmUInt64 frames, AmUInt16 channels, AmUInt32 sampleRate, bool isInterleaved)
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

        FilterInstance::ProcessChannel(buffer, channel, frames, channels, sampleRate, isInterleaved);
    }

    AmInt16 BiquadResonantFilterInstance::ProcessSample(AmInt16 sample, AmUInt16 channel, AmUInt32 sampleRate)
    {
        const AmInt32 x = sample;
        /* */ AmInt32 y;

        const AmInt32 x1 = _state[channel].x1;
        const AmInt32 x2 = _state[channel].x2;
        const AmInt32 y1 = _state[channel].y1;
        const AmInt32 y2 = _state[channel].y2;

        y = (_a0 * x + _a1 * x1 + _a2 * x2 - _b1 * y1 - _b2 * y2) >> kAmFixedPointBits;

        _state[channel].x1 = x;
        _state[channel].x2 = x1;
        _state[channel].y1 = y;
        _state[channel].y2 = y1;

        y = x + ((y - x) * AmFloatToFixedPoint(m_parameters[BiquadResonantFilter::ATTRIBUTE_WET]) >> kAmFixedPointBits);
        y = AM_CLAMP(y, INT16_MIN, INT16_MAX);

        return static_cast<AmInt16>(y);
    }

    void BiquadResonantFilterInstance::ComputeBiquadResonantParams()
    {
        _isDirty = false;

        const AmReal32 q = m_parameters[BiquadResonantFilter::ATTRIBUTE_RESONANCE];
        const AmReal32 omega = 2.0f * M_PI * m_parameters[BiquadResonantFilter::ATTRIBUTE_FREQUENCY] / static_cast<AmReal32>(_sampleRate);
        const AmReal32 sinOmega = std::sin(omega);
        const AmReal32 cosOmega = std::cos(omega);
        const AmReal32 A = std::pow(10, (m_parameters[BiquadResonantFilter::ATTRIBUTE_GAIN] / 40));

        AmReal32 scalar, alpha, beta;

        switch (static_cast<AmUInt32>(m_parameters[BiquadResonantFilter::ATTRIBUTE_TYPE]))
        {
        default:
        case BiquadResonantFilter::TYPE_LOW_PASS:
            alpha = sinOmega / (2.0f * q);
            scalar = 1.0f / (1.0f + alpha);
            _a0 = AmFloatToFixedPoint(0.5f * (1.0f - cosOmega) * scalar);
            _a1 = AmFloatToFixedPoint((1.0f - cosOmega) * scalar);
            _a2 = _a0;
            _b1 = AmFloatToFixedPoint(-2.0f * cosOmega * scalar);
            _b2 = AmFloatToFixedPoint((1.0f - alpha) * scalar);
            break;
        case BiquadResonantFilter::TYPE_HIGH_PASS:
            alpha = sinOmega / (2.0f * q);
            scalar = 1.0f / (1.0f + alpha);
            _a0 = AmFloatToFixedPoint(0.5f * (1.0f + cosOmega) * scalar);
            _a1 = AmFloatToFixedPoint(-(1.0f + cosOmega) * scalar);
            _a2 = _a0;
            _b1 = AmFloatToFixedPoint(-2.0f * cosOmega * scalar);
            _b2 = AmFloatToFixedPoint((1.0f - alpha) * scalar);
            break;
        case BiquadResonantFilter::TYPE_BAND_PASS:
            alpha = sinOmega / (2.0f * q);
            scalar = 1.0f / (1.0f + alpha);
            _a0 = AmFloatToFixedPoint(q * alpha * scalar);
            _a1 = AmFloatToFixedPoint(0);
            _a2 = -_a0;
            _b1 = AmFloatToFixedPoint(-2.0f * cosOmega * scalar);
            _b2 = AmFloatToFixedPoint((1.0f - alpha) * scalar);
            break;
        case BiquadResonantFilter::TYPE_PEAK:
            alpha = sinOmega / (2.0f * q);
            scalar = 1.0f / (1.0f + (alpha / A));
            _a0 = AmFloatToFixedPoint((1.0f + (alpha * A)) * scalar);
            _a1 = AmFloatToFixedPoint(-2.0f * cosOmega * scalar);
            _a2 = AmFloatToFixedPoint((1.0f - (alpha * A)) * scalar);
            _b1 = AmFloatToFixedPoint(-2.0f * cosOmega * scalar);
            _b2 = AmFloatToFixedPoint((1.0f - (alpha / A)) * scalar);
            break;
        case BiquadResonantFilter::TYPE_NOTCH:
            alpha = sinOmega / (2.0f * q);
            scalar = 1.0f / (1.0f + alpha);
            _a0 = AmFloatToFixedPoint(1.0f * scalar);
            _a1 = AmFloatToFixedPoint(-2.0f * cosOmega * scalar);
            _a2 = _a0;
            _b1 = AmFloatToFixedPoint(-2.0f * cosOmega * scalar);
            _b2 = AmFloatToFixedPoint((1.0f - alpha) * scalar);
            break;
        case BiquadResonantFilter::TYPE_LOW_SHELF:
            alpha = sinOmega / (2.0f * std::sqrt((A + 1.0f / A) * (1.0f / q - 1.0f) + 2.0f));
            beta = 2.0f * std::sqrt(A) * alpha;
            scalar = 1.0f / ((A + 1.0f) + (A - 1.0f) * cosOmega + beta);
            _a0 = AmFloatToFixedPoint((A * ((A + 1.0f) - (A - 1.0f) * cosOmega + beta)) * scalar);
            _a1 = AmFloatToFixedPoint((2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosOmega)) * scalar);
            _a2 = AmFloatToFixedPoint((A * ((A + 1.0f) - (A - 1.0f) * cosOmega - beta)) * scalar);
            _b1 = AmFloatToFixedPoint((-2.0f * ((A - 1.0f) + (A + 1.0f) * cosOmega)) * scalar);
            _b2 = AmFloatToFixedPoint(((A + 1.0f) + (A - 1.0f) * cosOmega - beta) * scalar);
            break;
        case BiquadResonantFilter::TYPE_HIGH_SHELF:
            alpha = sinOmega / (2.0f * std::sqrt((A + 1.0f / A) * (1.0f / q - 1.0f) + 2.0f));
            beta = 2.0f * std::sqrt(A) * alpha;
            scalar = 1.0f / ((A + 1.0f) - (A - 1.0f) * cosOmega + beta);
            _a0 = AmFloatToFixedPoint((A * ((A + 1.0f) + (A - 1.0f) * cosOmega + beta)) * scalar);
            _a1 = AmFloatToFixedPoint((-2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosOmega)) * scalar);
            _a2 = AmFloatToFixedPoint((A * ((A + 1.0f) + (A - 1.0f) * cosOmega - beta)) * scalar);
            _b1 = AmFloatToFixedPoint((2.0f * ((A - 1.0f) - (A + 1.0f) * cosOmega)) * scalar);
            _b2 = AmFloatToFixedPoint(((A + 1.0f) - (A - 1.0f) * cosOmega - beta) * scalar);
            break;
        }
    }
} // namespace SparkyStudios::Audio::Amplitude