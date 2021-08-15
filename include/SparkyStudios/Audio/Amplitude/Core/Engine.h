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

#ifndef SPARK_AUDIO_ENGINE_H
#define SPARK_AUDIO_ENGINE_H

#include <string>

#include <SparkyStudios/Audio/Amplitude/Core/Common.h>

#include <SparkyStudios/Audio/Amplitude/Math/HandmadeMath.h>

#include <SparkyStudios/Audio/Amplitude/Core/Bus.h>
#include <SparkyStudios/Audio/Amplitude/Core/Channel.h>
#include <SparkyStudios/Audio/Amplitude/Core/Listener.h>
#include <SparkyStudios/Audio/Amplitude/Core/Version.h>

namespace SparkyStudios::Audio::Amplitude
{
    class SoundCollection;
    struct EngineConfigDefinition;
    struct EngineInternalState;

    typedef SoundCollection* SoundHandle;

    /**
     * @brief The central class of  the library that manages the Listeners, Channels, and tracks all of the internal state.
     */
    class Engine
    {
    private:
        /**
         * @brief Construct an uninitialized Engine.
         */
        Engine();

    public:
        ~Engine();

        /**
         * @brief Initialize the audio engine.
         *
         * @param configFile the path to the file containing an EngineConfigDefinition flatbuffer binary.
         *
         * @return Whether initialization was successful.
         */
        bool Initialize(AmString configFile);

        /**
         * @brief Initialize the audio engine.
         *
         * @param config A pointer to a loaded EngineConfigDefinition object.
         *
         * @return Whether initialization was successful.
         */
        bool Initialize(const EngineConfigDefinition* config);

        /**
         * @brief Update audio volume per channel each frame.
         *
         * @param delta The number of elapsed seconds since the last frame.
         */
        void AdvanceFrame(AmTime delta);

        /**
         * @brief Load a sound bank from a file. Queue the sound files in that sound
         *        bank for loading. Call StartLoadingSoundFiles() to trigger loading
         *        of the sound files on a separate thread.
         *
         * @param filename The file containing the SoundBank flatbuffer binary data.
         * @return true Returns true on success
         */
        bool LoadSoundBank(const std::string& filename);

        /**
         * @brief Unload a sound bank.
         *
         * @param filename The file to unload.
         */
        void UnloadSoundBank(const std::string& filename);

        /**
         * @brief Kick off loading thread to load all sound files queued with
         *        LoadSoundBank().
         */
        void StartLoadingSoundFiles();

        /**
         * @brief Return true if all sound files have been loaded. Must call
         *        StartLoadingSoundFiles() first.
         */
        bool TryFinalize();

        /**
         * @brief Get a SoundHandle given its name as defined in its JSON data.
         *
         * @param name The unique name as defined in the JSON data.
         */
        [[nodiscard]] SoundHandle GetSoundHandle(const std::string& name) const;

        /**
         * @brief Get a SoundHandle given its SoundCollectionDef filename.
         *
         * @param name The filename containing the flatbuffer binary data.
         */
        [[nodiscard]] SoundHandle GetSoundHandleFromFile(const std::string& filename) const;

        /**
         * @brief Adjusts the gain on the master bus.
         *
         * @param gain the gain to apply to all buses.
         */
        void SetMasterGain(float gain);

        /**
         * @brief Get the master bus's current gain.
         *
         * @return the master bus's current gain.
         */
        [[nodiscard]] float GetMasterGain() const;

        /**
         * @brief Mutes the Engine completely.
         *
         * @param mute whether to mute or unmute the Engine.
         */
        void SetMute(bool mute);

        /**
         * @brief Whether the Engine is currently muted.
         *
         * @return Whether the Engine is currently muted.
         */
        bool GetMute() const;

        /**
         * @brief Pauses all playing sounds and streams.
         *
         * @param pause Whether to pause or unpause the Engine.
         */
        void Pause(bool pause);

        /**
         * @brief Initialize and return a Listener.
         *
         * @return An initialized Listener.
         */
        Listener AddListener();

        /**
         * @brief Remove a Listener.
         *
         * @param The Listener to be removed.
         */
        void RemoveListener(Listener* listener);

        /**
         * @brief Returns the named GetBus.
         *
         * @return The named GetBus.
         */
        Bus FindBus(AmString bus_name);

        /**
         * @brief Play a sound associated with the given sound_handle.
         *
         * @param sound_handle A handle to the sound to play.
         *
         * @return The channel the sound is played on. If the sound could not be
         *         played, an invalid Channel is returned.
         */
        Channel Play(SoundHandle sound_handle);

        /**
         * @brief Play a sound associated with the given sound_handle at the given
         *        location.
         *
         * @param sound_handle A handle to the sound to play.
         * @param location The location of the sound.
         *
         * @return The channel the sound is played on. If the sound could not be
         *         played, an invalid Channel is returned.
         */
        Channel Play(SoundHandle sound_handle, const hmm_vec3& location);

        /**
         * @brief Play a sound associated with the given sound_handle at the given
         *        location with the given gain.
         *
         * @param sound_handle A handle to the sound to play.
         * @param location The location of the sound.
         * @param gain The gain of the sound.
         *
         * @return The channel the sound is played on. If the sound could not be
         *         played, an invalid Channel is returned.
         */
        Channel Play(SoundHandle sound_handle, const hmm_vec3& location, float gain);

        /**
         * @brief Play a sound associated with the given sound name.
         *
         * Note: Playing a sound with its SoundHandle is faster than using the sound
         * name as using the name requires a map lookup internally.
         *
         * @param sound_name A handle to the sound to play.
         *
         * @return The channel the sound is played on. If the sound could not be
         *         played, an invalid Channel is returned.
         */
        Channel Play(const std::string& sound_name);

        /**
         * @brief Play a sound associated with the given sound name at the given
         *        location.
         *
         * Note: Playing a sound with its SoundHandle is faster than using the sound
         * name as using the name requires a map lookup internally.
         *
         * @param sound_name A handle to the sound to play.
         * @param location The location of the sound.
         *
         * @return The channel the sound is played on. If the sound could not be
         *         played, an invalid Channel is returned.
         */
        Channel Play(const std::string& sound_name, const hmm_vec3& location);

        /**
         * @brief Play a sound associated with the given sound name at the given
         *        location with the given gain.
         *
         * Note: Playing a sound with its SoundHandle is faster than using the sound
         * name as using the name requires a map lookup internally.
         *
         * @param sound_name A handle to the sound to play.
         * @param location The location of the sound.
         * @param gain The gain of the sound.
         *
         * @return The channel the sound is played on. If the sound could not be
         *         played, an invalid Channel is returned.
         */
        Channel Play(const std::string& sound_name, const hmm_vec3& location, float gain);

        /**
         * @brief Gets the version structure.
         *
         * @return The version string structure
         */
        [[nodiscard]] const struct Version* Version() const;

        /**
         * @brief Gets the current state of this Engine.
         *
         * @return The current state of this Engine.
         */
        [[nodiscard]] EngineInternalState* GetState() const
        {
            return _state;
        }

        [[nodiscard]] const EngineConfigDefinition* GetEngineConfigDefinition() const;

        /**
         * @brief Returns an unique instance of the Amplitude Engine.
         */
        [[nodiscard]] static Engine* GetInstance();

    private:
        // Hold the engine config file contents.
        std::string _config;

        EngineInternalState* _state;
    };

} // namespace SparkyStudios::Audio::Amplitude

#endif // SPARK_AUDIO_ENGINE_H
