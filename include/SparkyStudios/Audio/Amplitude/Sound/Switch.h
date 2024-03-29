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

#ifndef SS_AMPLITUDE_AUDIO_SWITCH_H
#define SS_AMPLITUDE_AUDIO_SWITCH_H

#include <SparkyStudios/Audio/Amplitude/Core/Asset.h>
#include <SparkyStudios/Audio/Amplitude/Core/Common.h>
#include <SparkyStudios/Audio/Amplitude/Core/RefCounter.h>

namespace SparkyStudios::Audio::Amplitude
{
    struct EngineInternalState;

    struct SwitchDefinition;

    /**
     * @brief A switch state.
     */
    struct AM_API_PUBLIC SwitchState
    {
        /**
         * @brief The ID of this switch state.
         *
         * This ID is unique only in the parent switch.
         */
        AmObjectID m_id;

        /**
         * @brief The name of this switch state.
         */
        AmString m_name;

        /**
         * @brief Checks whether this switch state is valid.
         *
         * @return @c true if the switch state is valid, @c false otherwise.
         */
        [[nodiscard]] bool Valid() const;

        bool operator==(const SwitchState& other) const;
        bool operator!=(const SwitchState& other) const;
    };

    /**
     * @brief Amplitude Switch.
     *
     * A switch is a collection of states which can change the sound played from a SwitchContainer.
     *
     * For example, you can have a switch named "SurfaceType" which have "wood", "grass", "metal" and "water" as states. A
     * SwitchContainer using this switch can group sounds per switch states, so when a state is active, all the sounds of
     * that state are played.
     *
     * The Switch is a shared object between sound sources. They are used only by SwitchContainer objects.
     */
    class AM_API_PUBLIC Switch final : public Asset<AmSwitchID, SwitchDefinition>
    {
    public:
        /**
         * @brief Creates an uninitialized Switch.
         *
         * An uninitialized Switch cannot set no provide the actual switch state.
         */
        Switch();

        /**
         * \brief Destroys the switch asset and releases all related resources.
         */
        ~Switch() override;

        /**
         * @brief Get the current state of the switch.
         *
         * @return The current state of the switch.
         */
        [[nodiscard]] const SwitchState& GetState() const;

        /**
         * @brief Set the current state of the switch.
         *
         * @param state The state to apply to the switch.
         */
        void SetState(const SwitchState& state);

        /**
         * @brief Set the current state of the switch using the state ID.
         *
         * @param id The ID of the state to apply. This ID should exist in the list
         * of switch states.
         */
        void SetState(AmObjectID id);

        /**
         * @brief Set the current state of the switch using the state name.
         *
         * @param name The name of the state to apply. This name should exist in the
         * list of switch states.
         */
        void SetState(const AmString& name);

        /**
         * @brief Get the list of available SwitchStates in this Switch.
         *
         * @return The list of available SwitchStates.
         */
        [[nodiscard]] const std::vector<SwitchState>& GetSwitchStates() const;

        bool LoadDefinition(const SwitchDefinition* definition, EngineInternalState* state) override;
        [[nodiscard]] const SwitchDefinition* GetDefinition() const override;

    private:
        SwitchState _activeState;
        std::vector<SwitchState> _states;

        RefCounter _refCounter;
    };
} // namespace SparkyStudios::Audio::Amplitude

#endif // SS_AMPLITUDE_AUDIO_SWITCH_H
