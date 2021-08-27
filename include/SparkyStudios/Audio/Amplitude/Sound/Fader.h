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

#ifndef SS_AMPLITUDE_AUDIO_FADER_H
#define SS_AMPLITUDE_AUDIO_FADER_H

#include <SparkyStudios/Audio/Amplitude/Core/Common.h>

namespace SparkyStudios::Audio::Amplitude
{
    class Mixer;

    /**
     * @brief Helper class to process faders.
     *
     * A fader is use to move a value to a specific target value
     * during an amount of time.
     */
    class Fader
    {
        friend class Mixer;

    public:
        Fader();

        /**
         * @brief Set up fader.
         *
         * @param from The start value.
         * @param to The target value.
         * @param time The duration of transition.
         */
        void Set(float from, float to, AmTime time);

        /**
         * @brief Get the current fading value.
         *
         * @param currentTime The time at which the value should be calculated.
         *
         * @return The current value.
         */
        float Get(AmTime currentTime);

        /**
         * @brief Get the current fading value.
         *
         * @param percentage The percentage of time elapsed. This should be in the range [0, 1].
         *
         * @return The current value.
         */
        virtual float Get(float percentage) = 0;

        /**
         * @brief Get the state of this Fader.
         *
         * @return The Fader state.
         */
        [[nodiscard]] AM_FADER_STATE GetState() const
        {
            return m_state;
        }

        /**
         * @brief Set the state of this Fader.
         *
         * @param state The state to set.
         */
        void SetState(AM_FADER_STATE state)
        {
            m_state = state;
        }

        /**
         * @brief Sets the fading start time.
         *
         * @param time The fading start time.
         */
        void Start(AmTime time);

        /**
         * @brief Creates a fader which fades linearly between two values.
         *
         * @return A linear fader.
         */
        static Fader* CreateLinear();

    protected:
        // Value to fade from
        float m_from;
        // Value to fade to
        float m_to;
        // Delta between from and to
        float m_delta;
        // Total AmTime to fade
        AmTime m_time;
        // Time fading started
        AmTime m_startTime;
        // Time fading will end
        AmTime m_endTime;
        // Active flag; 0 means disabled, 1 is active, -1 means was active, but stopped
        AM_FADER_STATE m_state;
    };
}; // namespace SparkyStudios::Audio::Amplitude

#endif // SS_AMPLITUDE_AUDIO_FADER_H
