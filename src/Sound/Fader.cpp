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

#include <SparkyStudios/Audio/Amplitude/Sound/Fader.h>

#include <Sound/Faders/ConstantFader.h>
#include <Sound/Faders/ExponentialFader.h>
#include <Sound/Faders/LinearFader.h>
#include <Sound/Faders/SCurveFader.h>

namespace SparkyStudios::Audio::Amplitude
{
    Fader::Fader()
    {
        m_from = m_to = m_delta = 0;
        m_time = m_startTime = m_endTime = 0;
        m_state = AM_FADER_STATE_DISABLED;
    }

    void Fader::Set(float from, float to, AmTime duration)
    {
        SetDuration(duration);
        Set(from, to);
        Start(0.0);
    }

    void Fader::Set(float from, float to)
    {
        m_from = from;
        m_to = to;
        m_delta = to - from;
    }

    void Fader::SetDuration(AmTime duration)
    {
        m_time = duration;
    }

    float Fader::GetFromTime(AmTime time)
    {
        if (m_state != AM_FADER_STATE_ACTIVE)
            return 0.0f;

        if (m_startTime > time)
            return m_from;

        if (time > m_endTime)
            return m_to;

        return GetFromPercentage((time - m_startTime) / (m_endTime - m_startTime));
    }

    void Fader::Start(AmTime time)
    {
        m_startTime = time;
        m_endTime = m_startTime + m_time;
        m_state = AM_FADER_STATE_ACTIVE;
    }

    Fader* Fader::Create(Fader::FADER_ALGORITHM algorithm)
    {
        switch (algorithm)
        {
        default:
        case Fader::ALGORITHM_LINEAR:
            return new LinearFader();
        case Fader::ALGORITHM_CONSTANT:
            return new ConstantFader();
        case Fader::ALGORITHM_S_CURVE:
            return new SCurveFader();
        case Fader::ALGORITHM_EXPONENTIAL:
            return new ExponentialFader();
        }
    }
} // namespace SparkyStudios::Audio::Amplitude