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

#include <SparkyStudios/Audio/Amplitude/Amplitude.h>

#include <Core/EngineInternalState.h>
#include <Mixer/SoundData.h>

#include "collection_definition_generated.h"
#include "sound_definition_generated.h"

namespace SparkyStudios::Audio::Amplitude
{
    static AmObjectID gLastSoundInstanceID = 0;

    Sound::Sound()
        : SoundObject()
        , m_format()
        , _decoder(nullptr)
        , _stream(false)
        , _loop(false)
        , _loopCount(0)
        , _source()
        , _settings()
        , _soundData(nullptr)
        , _soundDataRefCounter()
    {}

    Sound::~Sound()
    {
        if (_decoder != nullptr)
        {
            _decoder->Close();
            delete _decoder;
            _decoder = nullptr;
        }

        if (_soundData != nullptr)
        {
            SoundChunk::DestroyChunk(_soundData);
            _soundData = nullptr;
        }

        m_bus = nullptr;
        m_effect = nullptr;
        m_attenuation = nullptr;
    }

    bool Sound::LoadDefinition(const std::string& source, EngineInternalState* state)
    {
        // Ensure we don't load the sound more than once
        AMPLITUDE_ASSERT(m_id == kAmInvalidObjectId);

        _source = source;
        const SoundDefinition* definition = GetSoundDefinition();

        if (definition->id() == kAmInvalidObjectId)
        {
            CallLogFunc("[ERROR] Sound definition is invalid: no ID defined.");
            return false;
        }

        if (definition->bus() == kAmInvalidObjectId)
        {
            CallLogFunc("[ERROR] Sound definition is invalid: no bus ID defined.");
            return false;
        }

        m_bus = FindBusInternalState(state, definition->bus());
        if (!m_bus)
        {
            CallLogFunc("[ERROR] Sound %s specifies an unknown bus ID: %u.\n", definition->name(), definition->bus());
            return false;
        }

        if (definition->effect() != kAmInvalidObjectId)
        {
            if (const auto findIt = state->effect_map.find(definition->effect()); findIt != state->effect_map.end())
            {
                m_effect = findIt->second.get();
            }
            else
            {
                CallLogFunc("[ERROR] Sound definition is invalid: invalid effect ID \"%u\"", definition->effect());
                return false;
            }
        }

        if (definition->attenuation() != kAmInvalidObjectId)
        {
            if (const auto findIt = state->attenuation_map.find(definition->attenuation()); findIt != state->attenuation_map.end())
            {
                m_attenuation = findIt->second.get();
            }
            else
            {
                CallLogFunc("[ERROR] Sound definition is invalid: invalid attenuation ID \"%u\"", definition->attenuation());
                return false;
            }
        }

        m_id = definition->id();
        m_name = definition->name()->str();

        _stream = definition->stream();
        _loop = definition->loop() != nullptr && definition->loop()->enabled();
        _loopCount = definition->loop() ? definition->loop()->loop_count() : 0;

        m_gain = RtpcValue(definition->gain());
        m_priority = RtpcValue(definition->priority());

        _settings.m_id = definition->id();
        _settings.m_kind = SoundKind::Standalone;
        _settings.m_busID = definition->bus();
        _settings.m_effectID = definition->effect();
        _settings.m_attenuationID = definition->attenuation();
        _settings.m_spatialization = definition->spatialization();
        _settings.m_priority = m_priority;
        _settings.m_gain = m_gain;
        _settings.m_loop = _loop;
        _settings.m_loopCount = _loopCount;

        return true;
    }

    bool Sound::LoadDefinitionFromFile(const AmOsString& filename, EngineInternalState* state)
    {
        std::string source;
        return Amplitude::LoadFile(filename, &source) && LoadDefinition(source, state);
    }

    void Sound::AcquireReferences(EngineInternalState* state)
    {
        AMPLITUDE_ASSERT(m_id != kAmInvalidObjectId);

        if (m_effect)
        {
            m_effect->GetRefCounter()->Increment();
        }

        if (m_attenuation)
        {
            m_attenuation->GetRefCounter()->Increment();
        }
    }

    void Sound::ReleaseReferences(EngineInternalState* state)
    {
        AMPLITUDE_ASSERT(m_id != kAmInvalidObjectId);

        if (m_effect)
        {
            m_effect->GetRefCounter()->Decrement();
        }

        if (m_attenuation)
        {
            m_attenuation->GetRefCounter()->Decrement();
        }
    }

    const SoundDefinition* Sound::GetSoundDefinition() const
    {
        return Amplitude::GetSoundDefinition(_source.c_str());
    }

    void Sound::Load(const FileLoader* loader)
    {
        if (GetFilename().empty())
        {
            CallLogFunc("[ERROR] Cannot load the sound: the filename is empty.\n");
            return;
        }

        const std::filesystem::path& filename = GetFilename();

        const Codec* codec = Codec::FindCodecForFile(filename.c_str());
        if (codec == nullptr)
        {
            CallLogFunc("[ERROR] Cannot load the sound: unable to find codec for '" AM_OS_CHAR_FMT "'.\n", filename.c_str());
            return;
        }

        _decoder = codec->CreateDecoder();
        if (!_decoder->Open(filename.c_str()))
        {
            CallLogFunc("[ERROR] Cannot load the sound: unable to initialize a decoder for '" AM_OS_CHAR_FMT "'.\n", filename.c_str());
            return;
        }

        m_format = _decoder->GetFormat();
    }

    SoundInstance* Sound::CreateInstance()
    {
        AMPLITUDE_ASSERT(m_id != kAmInvalidObjectId);
        return new SoundInstance(this, _settings, m_effect);
    }

    SoundInstance* Sound::CreateInstance(const Collection* collection)
    {
        if (collection == nullptr)
            return CreateInstance();

        AMPLITUDE_ASSERT(m_id != kAmInvalidObjectId);

        auto* sound = new SoundInstance(this, collection->_soundSettings.at(m_id), collection->m_effect);
        sound->_collection = collection;

        return sound;
    }

    void Sound::SetFormat(const SoundFormat& format)
    {
        m_format = format;
    }

    const SoundFormat& Sound::GetFormat() const
    {
        return m_format;
    }

    bool Sound::IsLoop() const
    {
        return _loop;
    }

    bool Sound::IsStream() const
    {
        return _stream;
    }

    SoundChunk* Sound::AcquireSoundData()
    {
        if (_stream)
            return nullptr;

        if (_soundDataRefCounter.GetCount() == 0)
        {
            _soundData = SoundChunk::CreateChunk(m_format.GetFramesCount(), m_format.GetNumChannels());

            if (_decoder->Load(reinterpret_cast<AmInt16Buffer>(_soundData->buffer)) != m_format.GetFramesCount())
            {
                CallLogFunc("Could not load a sound instance. Unable to read data from the parent sound.\n");
                return nullptr;
            }
        }

        _soundDataRefCounter.Increment();
        return _soundData;
    }

    void Sound::ReleaseSoundData()
    {
        if (_stream)
            return;

        _soundDataRefCounter.Decrement();

        if (_soundDataRefCounter.GetCount() == 0)
        {
            SoundChunk::DestroyChunk(_soundData);
            _soundData = nullptr;
        }
    }

    SoundInstance::SoundInstance(Sound* parent, const SoundInstanceSettings& settings, const Effect* effect)
        : _userData(nullptr)
        , _channel(nullptr)
        , _parent(parent)
        , _collection(nullptr)
        , _settings(settings)
        , _currentLoopCount(0)
        , _effectInstance()
        , _obstruction(0.0f)
        , _occlusion(0.0f)
        , _id(++gLastSoundInstanceID)
    {
        if (effect != nullptr)
            _effectInstance = effect->CreateInstance();
    }

    SoundInstance::~SoundInstance()
    {
        Destroy();

        delete static_cast<SoundData*>(_userData);
        _userData = nullptr;

        delete _effectInstance;
        _effectInstance = nullptr;

        _parent = nullptr;
    }

    void SoundInstance::Load()
    {
        AMPLITUDE_ASSERT(Valid());

        const AmUInt16 channels = _parent->m_format.GetNumChannels();
        const AmUInt64 frames = _parent->m_format.GetFramesCount();

        if (_parent->_stream)
        {
            SoundChunk* chunk = SoundChunk::CreateChunk(amEngine->GetSamplesPerStream(), channels);
            SoundData* data = SoundData::CreateMusic(_parent->m_format, chunk, frames, this);

            if (!data)
            {
                CallLogFunc("Could not load a sound instance. Unable to read data from the parent sound.\n");
                return;
            }

            SetUserData(data);
        }
        else
        {
            SoundChunk* chunk = _parent->AcquireSoundData();
            SoundData* data = SoundData::CreateSound(_parent->m_format, chunk, frames, this);

            if (!data)
            {
                CallLogFunc("Could not load a sound instance. Unable to read data from the parent sound.\n");
                return;
            }

            SetUserData(data);
        }
    }

    const SoundInstanceSettings& SoundInstance::GetSettings() const
    {
        return _settings;
    }

    AmVoidPtr SoundInstance::GetUserData() const
    {
        return _userData;
    }

    void SoundInstance::SetUserData(AmVoidPtr userData)
    {
        _userData = userData;
    }

    AmUInt64 SoundInstance::GetAudio(AmUInt64 offset, AmUInt64 frames) const
    {
        AMPLITUDE_ASSERT(Valid());

        if (!_parent->_stream || _userData == nullptr)
            return 0;

        const auto* data = static_cast<SoundData*>(_userData);

        const AmUInt16 channels = _parent->m_format.GetNumChannels();

        AmUInt64 n, l = frames, o = offset, r = 0;
        auto* b = reinterpret_cast<AmInt16Buffer>(data->chunk->buffer);

        bool needFill = true;
        do
        {
            n = _parent->_decoder->Stream(b, o, l);
            r += n;

            // If we reached the end of the file but looping is enabled, then
            // seek back to the beginning of the file and fill the remaining part of the buffer.
            if (needFill = n < l && _parent->_loop && _parent->_decoder->Seek(0); needFill)
            {
                b += n * channels;
                l -= n;
                o = 0;
            }
        } while (needFill);

        return r;
    }

    void SoundInstance::Destroy()
    {
        AMPLITUDE_ASSERT(Valid());

        if (_userData != nullptr)
        {
            static_cast<SoundData*>(_userData)->Destroy(_parent->_stream);
            if (!_parent->_stream)
            {
                _parent->ReleaseSoundData();
            }
        }
    }

    bool SoundInstance::Valid() const
    {
        return _parent != nullptr;
    }

    void SoundInstance::SetChannel(RealChannel* channel)
    {
        _channel = channel;
    }

    RealChannel* SoundInstance::GetChannel() const
    {
        return _channel;
    }

    const Sound* SoundInstance::GetSound() const
    {
        return _parent;
    }

    const Collection* SoundInstance::GetCollection() const
    {
        return _collection;
    }

    AmUInt32 SoundInstance::GetCurrentLoopCount() const
    {
        return _currentLoopCount;
    }

    const EffectInstance* SoundInstance::GetEffect() const
    {
        return _effectInstance;
    }

    void SoundInstance::SetObstruction(AmReal32 obstruction)
    {
        _obstruction = obstruction;
    }

    void SoundInstance::SetOcclusion(AmReal32 occlusion)
    {
        _occlusion = occlusion;
    }

    AmReal32 SoundInstance::GetObstruction() const
    {
        return _obstruction;
    }

    AmReal32 SoundInstance::GetOcclusion() const
    {
        return _occlusion;
    }

    AmObjectID SoundInstance::GetId() const
    {
        return _id;
    }
} // namespace SparkyStudios::Audio::Amplitude
