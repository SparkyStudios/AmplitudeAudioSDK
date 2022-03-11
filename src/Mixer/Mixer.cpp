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
#include <Mixer/Mixer.h>
#include <Utils/Utils.h>
#include <Utils/miniaudio/miniaudio_utils.h>

#define AMPLIMIX_STORE(A, C) std::atomic_store_explicit(A, C, std::memory_order_release)
#define AMPLIMIX_LOAD(A) std::atomic_load_explicit(A, std::memory_order_acquire)
#define AMPLIMIX_CSWAP(A, E, C) std::atomic_compare_exchange_strong_explicit(A, E, C, std::memory_order_acq_rel, std::memory_order_acquire)

namespace SparkyStudios::Audio::Amplitude
{
#if defined(AM_SSE_INTRINSICS)
    constexpr AmUInt32 kSimdProcessedFramesCount = (AudioDataUnit::length);
    constexpr AmUInt32 kSimdProcessedFramesCountHalf = (kSimdProcessedFramesCount / 2);
#endif // AM_SSE_INTRINSICS

    static void OnSoundDestroyed(Mixer* mixer, const MixerLayer* layer);

    static void* ma_malloc(size_t sz, void* pUserData)
    {
        return amMemory->Malloc(MemoryPoolKind::Amplimix, sz);
    }

    static void* ma_realloc(void* p, size_t sz, void* pUserData)
    {
        return amMemory->Realloc(MemoryPoolKind::Amplimix, p, sz);
    }

    static void ma_free(void* p, void* pUserData)
    {
        amMemory->Free(MemoryPoolKind::Amplimix, p);
    }

    static hmm_vec2 LRGain(float gain, float pan)
    {
        // Clamp pan to its valid range of -1.0f to 1.0f inclusive
        pan = AM_CLAMP(pan, -1.0f, 1.0f);

        // Convert gain and pan to left and right gain and store it atomically
        // This formula is explained in the following paper:
        // http://www.rs-met.com/documents/tutorials/PanRules.pdf
        const float p = (static_cast<float>(M_PI) * (pan + 1.0f)) / 4.0f;
        const float left = std::cos(p) * gain;
        const float right = std::sin(p) * gain;

        return { left, right };
    }

    static void OnSoundStarted(Mixer* mixer, const MixerLayer* layer)
    {
        const auto* sound = static_cast<SoundInstance*>(layer->snd->userData);
        CallLogFunc("Started sound: " AM_OS_CHAR_FMT "\n", sound->GetSound()->GetFilename().c_str());
    }

    static void OnSoundPaused(Mixer* mixer, const MixerLayer* layer)
    {
        const auto* sound = static_cast<SoundInstance*>(layer->snd->userData);
        CallLogFunc("Paused sound: " AM_OS_CHAR_FMT "\n", sound->GetSound()->GetFilename().c_str());
    }

    static void OnSoundResumed(Mixer* mixer, const MixerLayer* layer)
    {
        const auto* sound = static_cast<SoundInstance*>(layer->snd->userData);
        CallLogFunc("Resumed sound: " AM_OS_CHAR_FMT "\n", sound->GetSound()->GetFilename().c_str());
    }

    static void OnSoundStopped(Mixer* mixer, const MixerLayer* layer)
    {
        const auto* sound = static_cast<SoundInstance*>(layer->snd->userData);
        CallLogFunc("Stopped sound: " AM_OS_CHAR_FMT "\n", sound->GetSound()->GetFilename().c_str());
    }

    static bool OnSoundLooped(Mixer* mixer, const MixerLayer* layer)
    {
        auto* sound = static_cast<SoundInstance*>(layer->snd->userData);
        CallLogFunc("Looped sound: " AM_OS_CHAR_FMT "\n", sound->GetSound()->GetFilename().c_str());

        Mixer::IncrementSoundLoopCount(sound);

        if (const AmUInt32 loopCount = sound->GetSettings().m_loopCount; sound->GetCurrentLoopCount() == loopCount)
        {
            sound->GetChannel()->Halt();
            return false;
        }

        return true;
    }

    static AmUInt64 OnSoundStream(Mixer* mixer, const MixerLayer* layer, AmUInt64 offset, AmUInt64 frames)
    {
        if (layer->snd->stream)
        {
            const auto* sound = static_cast<SoundInstance*>(layer->snd->userData);
            return sound->GetAudio(offset, frames);
        }

        return 0;
    }

    static void OnSoundEnded(Mixer* mixer, const MixerLayer* layer)
    {
        auto* sound = static_cast<SoundInstance*>(layer->snd->userData);
        CallLogFunc("Ended sound: " AM_OS_CHAR_FMT "\n", sound->GetSound()->GetFilename().c_str());

        RealChannel* channel = sound->GetChannel();

        // Clean up the pipeline
        mixer->GetPipeline()->Cleanup(sound);

        if (const Engine* engine = Engine::GetInstance(); engine->GetState()->stopping)
            goto Delete;

        if (sound->GetSettings().m_kind == SoundKind::Standalone)
        {
            goto Stop;
        }
        else if (sound->GetSettings().m_kind == SoundKind::Switched)
        {
            goto Delete;
        }
        else if (sound->GetSettings().m_kind == SoundKind::Contained)
        {
            const Collection* collection = sound->GetCollection();
            AMPLITUDE_ASSERT(collection != nullptr); // Should always have a collection for contained sound instances.

            if (const CollectionDefinition* config = collection->GetCollectionDefinition();
                config->play_mode() == CollectionPlayMode_PlayAll)
            {
                if (channel->Valid())
                {
                    channel->MarkAsPlayed(sound->GetSound());
                    if (channel->AllSoundsHasPlayed())
                    {
                        channel->ClearPlayedSounds();
                        if (config->play_mode() == CollectionPlayMode_PlayAll)
                        {
                            goto Stop;
                        }
                    }

                    // Play the collection again only if the channel is still playing.
                    if (channel->Playing())
                    {
                        channel->GetParentChannelState()->Play();
                    }
                }

                // Delete the current sound instance.
                goto Delete;
            }
        }
        else
        {
            AMPLITUDE_ASSERT(false); // Should never fall in this case.
        }

    Stop:
        // Stop playing the sound
        channel->GetParentChannelState()->Halt();

    Delete:
        // Delete the sound instance at the end of the playback
        OnSoundDestroyed(mixer, layer);
    }

    static void OnSoundDestroyed(Mixer* mixer, const MixerLayer* layer)
    {
        MixerCommandCallback callback = [layer]() -> bool
        {
            auto* sound = static_cast<SoundInstance*>(layer->snd->userData);
            delete sound;
            return true;
        };

        if (mixer->IsInsideThreadMutex())
        {
            mixer->PushCommand({ callback });
            return;
        }

        callback();
    }

    static void MixMono(Mixer* mixer, AmUInt64 index, const AudioDataUnit& gain, SoundChunk* in, AudioBuffer outBuffer)
    {
#if defined(AM_SSE_INTRINSICS)
        outBuffer[index] =
            simdpp::add(outBuffer[index], simdpp::to_int16(simdpp::shift_r(simdpp::mull(in->buffer[index], gain), kAmFixedPointBits)));
#else
        outBuffer[index] += static_cast<AmInt16>((in->buffer[index] * lGain) >> kAmFixedPointBits);
#endif // AM_SSE_INTRINSICS
    }

    static void MixStereo(
        Mixer* mixer, AmUInt64 index, const AudioDataUnit& lGain, const AudioDataUnit& rGain, SoundChunk* in, AudioBuffer outBuffer)
    {
#if defined(AM_SSE_INTRINSICS)
        outBuffer[index + 0] = simdpp::add(
            outBuffer[index + 0], simdpp::to_int16(simdpp::shift_r(simdpp::mull(in->buffer[index + 0], lGain), kAmFixedPointBits)));
        outBuffer[index + 1] = simdpp::add(
            outBuffer[index + 1], simdpp::to_int16(simdpp::shift_r(simdpp::mull(in->buffer[index + 1], rGain), kAmFixedPointBits)));
#else
        outBuffer[index + 0] += static_cast<AmInt16>((in->buffer[index + 0] * lGain) >> kAmFixedPointBits);
        outBuffer[index + 1] += static_cast<AmInt16>((in->buffer[index + 1] * rGain) >> kAmFixedPointBits);
#endif // AM_SSE_INTRINSICS
    }

    Mixer::Mixer(float masterGain)
        : _initialized(false)
        , _commandsStack()
        , _audioThreadMutex(nullptr)
        , _insideAudioThreadMutex(false)
        , _nextId(0)
        , _layers{}
        , _remainingFrames(0)
        , _pipeline(nullptr)
        , _device()
    {
        AMPLIMIX_STORE(&_masterGain, masterGain);
    }

    Mixer::~Mixer()
    {
        AMPLITUDE_ASSERT(!_insideAudioThreadMutex);

        if (_initialized)
        {
            _initialized = false;

            if (_audioThreadMutex)
                Thread::DestroyMutex(_audioThreadMutex);

            _audioThreadMutex = nullptr;
        }
    }

    bool Mixer::Init(const EngineConfigDefinition* config)
    {
        if (_initialized)
        {
            CallLogFunc("Amplimix has already been initialized.\n");
            return false;
        }

        _device.mOutputBufferSize = config->output()->buffer_size();
        _device.mRequestedOutputSampleRate = config->output()->frequency();
        _device.mRequestedOutputChannels = static_cast<PlaybackOutputChannels>(config->output()->channels());
        _device.mRequestedOutputFormat = static_cast<PlaybackOutputFormat>(config->output()->format());

        _audioThreadMutex = Thread::CreateMutexAm();

        if (const auto* pipeline = config->mixer()->pipeline(); pipeline != nullptr && pipeline->size() > 0)
        {
            _pipeline = new ProcessorPipeline();

            for (flatbuffers::uoffset_t i = 0, l = pipeline->size(); i < l; ++i)
            {
                switch (config->mixer()->pipeline_type()->Get(i))
                {
                case AudioMixerPipelineItem_AudioProcessorMixer:
                    {
                        const auto* p = pipeline->GetAs<AudioProcessorMixer>(i);
                        SoundProcessor* dryProcessor = SoundProcessor::Find(p->dry_processor()->str());
                        SoundProcessor* wetProcessor = SoundProcessor::Find(p->wet_processor()->str());

                        if (dryProcessor == nullptr)
                        {
                            CallLogFunc(
                                "[WARNING] Unable to find a registered sound processor with name: %s\n", p->dry_processor()->c_str());
                            continue;
                        }

                        if (wetProcessor == nullptr)
                        {
                            CallLogFunc(
                                "[WARNING] Unable to find a registered sound processor with name: %s\n", p->wet_processor()->c_str());
                            continue;
                        }

                        auto* mixer = new ProcessorMixer();
                        mixer->SetDryProcessor(dryProcessor, p->dry());
                        mixer->SetWetProcessor(wetProcessor, p->wet());

                        _pipeline->Append(mixer);
                    }
                    break;

                case AudioMixerPipelineItem_AudioSoundProcessor:
                    {
                        const auto* p = pipeline->GetAs<AudioSoundProcessor>(i);
                        SoundProcessor* soundProcessor = SoundProcessor::Find(p->processor()->str());
                        if (soundProcessor == nullptr)
                        {
                            CallLogFunc("[WARNING] Unable to find a registered sound processor with name: %s\n", p->processor()->c_str());
                            continue;
                        }

                        _pipeline->Append(soundProcessor);
                    }
                    break;
                }
            }
        }

        _initialized = true;

        return true;
    }

    void Mixer::PostInit(
        AmObjectID deviceID,
        std::string deviceName,
        AmUInt32 deviceOutputSampleRate,
        PlaybackOutputChannels deviceOutputChannels,
        PlaybackOutputFormat deviceOutputFormat)
    {
        _device.mDeviceID = deviceID;
        _device.mDeviceName = std::move(deviceName);
        _device.mDeviceOutputSampleRate = deviceOutputSampleRate;
        _device.mDeviceOutputChannels = deviceOutputChannels;
        _device.mDeviceOutputFormat = deviceOutputFormat;
    }

    AmUInt64 Mixer::Mix(AmVoidPtr mixBuffer, AmUInt64 frameCount)
    {
        LockAudioMutex();

        const auto numChannels = static_cast<AmUInt16>(_device.mRequestedOutputChannels);
#if defined(AM_SSE_INTRINSICS)
        const auto lower = simdpp::make_int<AudioDataUnit>(INT16_MIN), upper = simdpp::make_int<AudioDataUnit>(INT16_MAX);
#else
        const AudioDataUnit lower = INT16_MIN, upper = INT16_MAX;
#endif // AM_SSE_INTRINSICS

        auto* buffer = static_cast<AmInt16Buffer>(mixBuffer);
        memset(buffer, 0, frameCount * numChannels * sizeof(AmInt16));

        // output remaining frames in buffer before mixing new ones
        AmUInt64 frames = frameCount;

        // dynamically sized buffer
        auto* align = SoundChunk::CreateChunk(frames, numChannels, MemoryPoolKind::Amplimix);

#if defined(AM_SSE_INTRINSICS)
        // aSize in Vc::int16_v and multiple of kSimdProcessedFramesCountHalf
        const AmUInt64 aSize = frames / align->samplesPerVector;

        // determine remaining number of frames
        _remainingFrames = aSize * align->samplesPerVector - frames;
#else
        // aSize in AmInt16
        const AmUInt64 aSize = frames * numChannels;

        _remainingFrames = 0; // Should not have remaining frames without SSE optimization
#endif // AM_SSE_INTRINSICS

        // begin actual mixing
        bool hasMixedAtLeastOneLayer = false;
        for (auto&& layer : _layers)
        {
            if (ShouldMix(&layer))
            {
                UpdatePitch(&layer);

                hasMixedAtLeastOneLayer = true;
                MixLayer(&layer, align->buffer, aSize, frames);

                // If we have mixed more frames than required, move back the cursor
                if (_remainingFrames)
                {
                    AmUInt64 cursor = AMPLIMIX_LOAD(&layer.cursor);
                    cursor -= _remainingFrames;
                    AMPLIMIX_STORE(&layer.cursor, cursor);
                }
            }
        }

        if (!hasMixedAtLeastOneLayer)
            goto Cleanup;

        // perform clipping using min and max
        for (AmUInt32 i = 0; i < aSize; i++)
        {
#if defined(AM_SSE_INTRINSICS)
            align->buffer[i] = simdpp::min(simdpp::max(align->buffer[i].eval(), lower).eval(), upper).eval();
#else
            align->buffer[i] = std::min(std::max(align[i], lower), upper);
#endif // AM_SSE_INTRINSICS
        }

        // copy frames, leaving possible remainder
        memcpy(buffer, reinterpret_cast<AmInt16Buffer>(align->buffer), frames * numChannels * sizeof(AmInt16));

    Cleanup:
        SoundChunk::DestroyChunk(align);

        UnlockAudioMutex();

        ExecuteCommands();

        return frameCount;
    }

    AmUInt32 Mixer::Play(
        SoundData* sound, PlayStateFlag flag, AmReal32 gain, AmReal32 pan, AmReal32 pitch, AmReal32 speed, AmUInt32 id, AmUInt32 layer)
    {
        return PlayAdvanced(sound, flag, gain, pan, pitch, speed, 0, sound->length, id, layer);
    }

    AmUInt32 Mixer::PlayAdvanced(
        SoundData* sound,
        PlayStateFlag flag,
        AmReal32 gain,
        AmReal32 pan,
        AmReal32 pitch,
        AmReal32 speed,
        AmUInt64 startFrame,
        AmUInt64 endFrame,
        AmUInt32 id,
        AmUInt32 layer)
    {
        if (flag <= PLAY_STATE_FLAG_MIN || flag >= PLAY_STATE_FLAG_MAX)
            return 0; // invalid flag

#if defined(AM_SSE_INTRINSICS)
        if (endFrame - startFrame < kSimdProcessedFramesCount || endFrame < kSimdProcessedFramesCount)
            return 0; // invalid frame range
#endif // AM_SSE_INTRINSICS

        // define a layer id
        layer = layer == 0 ? ++_nextId : layer;

        // skip 0 as it is special
        if (id == 0)
            id = kAmplimixLayersCount;

        MixerCommandCallback callback = [=]() -> bool
        {
            // get layer for next sound handle id
            auto* lay = GetLayer(layer);

            // check if corresponding layer is free
            if (AMPLIMIX_LOAD(&lay->flag) == PLAY_STATE_FLAG_MIN)
            {
                // fill in non-atomic layer data along with truncating start and end
                lay->id = id;
                lay->snd = sound;

#if defined(AM_SSE_INTRINSICS)
                lay->start = startFrame & ~(kSimdProcessedFramesCount - 1);
                lay->end = endFrame & ~(kSimdProcessedFramesCount - 1);
#else
                lay->start = startFrame;
                lay->end = endFrame;
#endif // AM_SSE_INTRINSICS

                // Initialize the sample rate converter
                if (lay->sampleRateConverter != nullptr)
                    src_delete(lay->sampleRateConverter);

                lay->sampleRateConverter =
                    src_new(amEngine->GetSampleRateConversionQuality(), static_cast<AmUInt16>(_device.mRequestedOutputChannels), nullptr);

                if (lay->sampleRateConverter == nullptr)
                    return false;

                // convert gain and pan to left and right gain and store it atomically
                AMPLIMIX_STORE(&lay->gain, LRGain(gain, pan));
                // store the pitch
                AMPLIMIX_STORE(&lay->pitch, pitch);
                // store the playback speed
                AMPLIMIX_STORE(&lay->userPlaySpeed, speed);
                // atomically set cursor to start position based on given argument
                AMPLIMIX_STORE(&lay->cursor, lay->start);
                // store flag last, releasing the layer to the mixer thread
                AMPLIMIX_STORE(&lay->flag, flag);

                // return success
                return true;
            }

            return false;
        };

        if (_insideAudioThreadMutex)
        {
            PushCommand({ callback });
            return layer;
        }

        if (callback())
            return layer;

        return 0;
    }

    bool Mixer::SetGainPan(AmUInt32 id, AmUInt32 layer, float gain, float pan)
    {
        auto* lay = GetLayer(layer);

        // check id and state flag to make sure the id is valid
        if ((id == lay->id) && (AMPLIMIX_LOAD(&lay->flag) > PLAY_STATE_FLAG_STOP))
        {
            if (lay->snd != nullptr && lay->snd->format.GetNumChannels() == 1)
                pan = 0.0f;

            // convert gain and pan to left and right gain and store it atomically
            AMPLIMIX_STORE(&lay->gain, LRGain(gain, pan));
            // return success
            return true;
        }

        // return failure
        return false;
    }

    bool Mixer::SetPitch(AmUInt32 id, AmUInt32 layer, AmReal32 pitch)
    {
        auto* lay = GetLayer(layer);

        // check id and state flag to make sure the id is valid
        if ((id == lay->id) && (AMPLIMIX_LOAD(&lay->flag) > PLAY_STATE_FLAG_STOP))
        {
            // convert gain and pan to left and right gain and store it atomically
            AMPLIMIX_STORE(&lay->pitch, pitch);
            // return success
            return true;
        }

        // return failure
        return false;
    }

    bool Mixer::SetCursor(AmUInt32 id, AmUInt32 layer, AmUInt64 cursor)
    {
        auto* lay = GetLayer(layer);

        // check id and state flag to make sure the id is valid
        if ((id == lay->id) && (AMPLIMIX_LOAD(&lay->flag) > PLAY_STATE_FLAG_STOP))
        {
#if defined(AM_SSE_INTRINSICS)
            // clamp cursor and truncate to multiple of 16 before storing
            AMPLIMIX_STORE(&lay->cursor, AM_CLAMP(cursor, lay->start, lay->end) & ~(kSimdProcessedFramesCount - 1));
#else
            // clamp cursor and store it
            AMPLIMIX_STORE(&lay->cursor, AM_CLAMP(cursor, lay->start, lay->end));
#endif // AM_SSE_INTRINSICS

            // return success
            return true;
        }

        // return failure
        return false;
    }

    bool Mixer::SetPlayState(AmUInt32 id, AmUInt32 layer, PlayStateFlag flag)
    {
        // return failure if given flag invalid
        if (flag >= PLAY_STATE_FLAG_MAX)
            return false;

        MixerCommandCallback callback = [=]() -> bool
        {
            // get layer based on the lowest bits of id
            auto* lay = GetLayer(layer);

            // check id and state flag to make sure the id is valid
            if (PlayStateFlag prev; (id == lay->id) && ((prev = AMPLIMIX_LOAD(&lay->flag)) >= PLAY_STATE_FLAG_STOP))
            {
                // return success if already in desired state
                if (prev == flag)
                    return true;

                // run appropriate callback
                if (prev == PLAY_STATE_FLAG_STOP && (flag == PLAY_STATE_FLAG_PLAY || flag == PLAY_STATE_FLAG_LOOP))
                    OnSoundStarted(this, lay);
                else if ((prev == PLAY_STATE_FLAG_PLAY || prev == PLAY_STATE_FLAG_LOOP) && flag == PLAY_STATE_FLAG_HALT)
                    OnSoundPaused(this, lay);
                else if (prev == PLAY_STATE_FLAG_HALT && (flag == PLAY_STATE_FLAG_PLAY || flag == PLAY_STATE_FLAG_LOOP))
                    OnSoundResumed(this, lay);
                else if ((prev == PLAY_STATE_FLAG_PLAY || prev == PLAY_STATE_FLAG_LOOP) && flag == PLAY_STATE_FLAG_STOP)
                    OnSoundStopped(this, lay);

                // swap if flag has not changed and return if successful
                if (AMPLIMIX_CSWAP(&lay->flag, &prev, flag))
                    return true;
            }

            // return failure
            return false;
        };

        if (_insideAudioThreadMutex)
        {
            PushCommand({ callback });
            return true;
        }

        return callback();
    }

    PlayStateFlag Mixer::GetPlayState(AmUInt32 id, AmUInt32 layer)
    {
        // get layer based on the lowest bits of id
        auto* lay = GetLayer(layer);

        // check id and state flag to make sure the id is valid
        if (PlayStateFlag flag; (id == lay->id) && ((flag = AMPLIMIX_LOAD(&lay->flag)) > PLAY_STATE_FLAG_STOP))
        {
            // return the found flag
            return flag;
        }

        // return failure
        return PLAY_STATE_FLAG_MIN;
    }

    bool Mixer::SetPlaySpeed(AmUInt32 id, AmUInt32 layer, AmReal32 speed)
    {
        auto* lay = GetLayer(layer);

        // check id and state flag to make sure the id is valid
        if ((id == lay->id) && (AMPLIMIX_LOAD(&lay->flag) > PLAY_STATE_FLAG_STOP))
        {
            // convert gain and pan to left and right gain and store it atomically
            AMPLIMIX_STORE(&lay->userPlaySpeed, speed);
            // return success
            return true;
        }

        // return failure
        return false;
    }

    void Mixer::SetMasterGain(float gain)
    {
        AMPLIMIX_STORE(&_masterGain, gain);
    }

    void Mixer::StopAll()
    {
        // go through all active layers and set their states to the stop state
        for (auto&& lay : _layers)
        {
            // check if active and set to stop if true
            if (AMPLIMIX_LOAD(&lay.flag) > PLAY_STATE_FLAG_STOP)
                AMPLIMIX_STORE(&lay.flag, PLAY_STATE_FLAG_STOP);
        }
    }

    void Mixer::HaltAll()
    {
        // go through all playing layers and set their states to halt
        for (auto&& lay : _layers)
        {
            // check if playing or looping and try to swap
            if (PlayStateFlag flag; (flag = AMPLIMIX_LOAD(&lay.flag)) > PLAY_STATE_FLAG_HALT)
                AMPLIMIX_CSWAP(&lay.flag, &flag, PLAY_STATE_FLAG_HALT);
        }
    }

    void Mixer::PlayAll()
    {
        // go through all halted layers and set their states to play
        for (auto&& lay : _layers)
        {
            // need to reset each time
            PlayStateFlag flag = PLAY_STATE_FLAG_HALT;
            // swap the flag to play if it is on halt
            AMPLIMIX_CSWAP(&lay.flag, &flag, PLAY_STATE_FLAG_PLAY);
        }
    }

    bool Mixer::IsInsideThreadMutex() const
    {
        return _insideAudioThreadMutex;
    }

    void Mixer::PushCommand(const MixerCommand& command)
    {
        _commandsStack.push(command);
    }

    const ProcessorPipeline* Mixer::GetPipeline() const
    {
        return _pipeline;
    }

    ProcessorPipeline* Mixer::GetPipeline()
    {
        return _pipeline;
    }

    void Mixer::IncrementSoundLoopCount(SoundInstance* sound)
    {
        ++sound->_currentLoopCount;
    }

    const DeviceDescription& Mixer::GetDeviceDescription() const
    {
        return _device;
    }

    void Mixer::ExecuteCommands()
    {
        while (!_commandsStack.empty())
        {
            const auto& command = _commandsStack.front();
            if (command.callback)
            {
                command.callback();
            }
            _commandsStack.pop();
        }
    }

    void Mixer::MixLayer(MixerLayer* layer, AudioBuffer buffer, AmUInt64 bufferSize, AmUInt64 samples)
    {
        if (_pipeline == nullptr)
        {
            CallLogFunc("[WARNING] No active pipeline is set, this means no sound will be rendered. You should configure the Amplimix "
                        "pipeline in your engine configuration file.\n");
            return;
        }

        const auto reqChannels = static_cast<AmUInt16>(_device.mRequestedOutputChannels);

        // load flag value atomically first
        PlayStateFlag flag = AMPLIMIX_LOAD(&layer->flag);

        // atomically load cursor
        AmUInt64 cursor = AMPLIMIX_LOAD(&layer->cursor);

        // atomically load left and right gain
        const hmm_vec2 g = AMPLIMIX_LOAD(&layer->gain);
        const float gain = AMPLIMIX_LOAD(&_masterGain);
#if defined(AM_SSE_INTRINSICS)
        const auto mxGain = simdpp::make_int<AudioDataUnit>(AmFloatToFixedPoint(g.X * gain), AmFloatToFixedPoint(g.Y * gain));
        const auto& lGain = mxGain;
        const auto& rGain = mxGain;
#else
        const auto lGain = AmFloatToFixedPoint(g.X * gain);
        const auto rGain = AmFloatToFixedPoint(g.Y * gain);
#endif // AM_SSE_INTRINSICS

        // loop state
        const bool loop = (flag == PLAY_STATE_FLAG_LOOP);

        const AmUInt16 soundChannels = layer->snd->format.GetNumChannels();
        const AmUInt32 baseSampleRate = layer->snd->format.GetSampleRate();
        const AmUInt32 sampleRate = AMPLIMIX_LOAD(&layer->sampleRate);
        /* */ AmReal32 step = static_cast<AmReal32>(baseSampleRate) / static_cast<AmReal32>(sampleRate);

        // avoid step overflow
        if (step > (1 << (32 - kAmFixedPointBits)))
            step = 0;

        const AmUInt64 outSamples = samples;
        /* */ AmUInt64 inSamples = std::ceil(step * static_cast<AmReal32>(outSamples));
#if defined(AM_SSE_INTRINSICS)
        inSamples = AM_VALUE_ALIGN(inSamples, kSimdProcessedFramesCount);
#endif // AM_SSE_INTRINSICS

        SoundChunk* in = SoundChunk::CreateChunk(inSamples, soundChannels, MemoryPoolKind::Amplimix);

        // if this sound is streaming, and we have a stream event callback
        if (layer->snd != nullptr && layer->snd->stream)
        {
            // mix sound per chunk of streamed data
            AmUInt64 c = inSamples;
            while (c > 0 && flag != PLAY_STATE_FLAG_MIN)
            {
                // update flag value
                flag = AMPLIMIX_LOAD(&layer->flag);

                if (flag == PLAY_STATE_FLAG_MIN)
                    break;

                const AmUInt64 chunkSize = AM_MIN(layer->snd->chunk->frames, c);
                /* */ AmUInt64 readLen = chunkSize;

#if defined(AM_SSE_INTRINSICS)
                readLen = AM_VALUE_ALIGN(readLen, kSimdProcessedFramesCount);
#endif // AM_SSE_INTRINSICS

                readLen = OnSoundStream(this, layer, (cursor + (inSamples - c)) % layer->snd->length, readLen);
                readLen = AM_MIN(readLen, chunkSize);

                // having 0 here mainly means that we have reached
                // the end of the stream and the audio is not looping.
                if (readLen == 0)
                    break;

                memcpy(
                    reinterpret_cast<AmInt16Buffer>(in->buffer) + ((inSamples - c) * soundChannels),
                    reinterpret_cast<AmInt16Buffer>(layer->snd->chunk->buffer), readLen * soundChannels * sizeof(AmInt16));

                c -= readLen;
            }
        }
        else if (layer->snd != nullptr && !layer->snd->stream)
        {
            // Compute offset
            const AmUInt64 offset = (cursor % layer->snd->length) * soundChannels;
            const AmUInt64 remaining = layer->snd->chunk->frames - cursor;

            if (cursor < layer->snd->chunk->frames && remaining < outSamples)
            {
                const AmUInt64 size = remaining * layer->snd->format.GetFrameSize();

                memcpy(
                    reinterpret_cast<AmInt16Buffer>(in->buffer), reinterpret_cast<AmInt16Buffer>(layer->snd->chunk->buffer) + offset, size);

                memcpy(
                    reinterpret_cast<AmInt16Buffer>(in->buffer) + (remaining * soundChannels),
                    reinterpret_cast<AmInt16Buffer>(layer->snd->chunk->buffer), in->size - size);
            }
            else
            {
                memcpy(
                    reinterpret_cast<AmInt16Buffer>(in->buffer), reinterpret_cast<AmInt16Buffer>(layer->snd->chunk->buffer) + offset,
                    in->size);
            }
        }

        // action based on flag
        if (flag >= PLAY_STATE_FLAG_PLAY)
        {
            // Cache cursor
            AmUInt64 oldCursor = cursor;

            const AmInt32 stepFixed = AmFloatToFixedPoint(step);

            SoundChunk* out = SoundChunk::CreateChunk(AM_MAX(inSamples, outSamples), reqChannels, MemoryPoolKind::Amplimix);

            if (reqChannels != soundChannels)
            {
                auto* channelMap = static_cast<ma_channel*>(amMemory->Malloc(MemoryPoolKind::Amplimix, reqChannels * sizeof(ma_channel)));
                ma_channel_map_init_standard(ma_standard_channel_map_vorbis, channelMap, reqChannels, reqChannels);

                ma_channel_converter converter{};

                ma_channel_converter_config config = ma_channel_converter_config_init(
                    ma_format_from_amplitude(_device.mRequestedOutputFormat), soundChannels, nullptr, reqChannels, channelMap,
                    ma_channel_mix_mode_default);

                ma_allocation_callbacks allocationCallbacks{};
                allocationCallbacks.onFree = ma_free;
                allocationCallbacks.onMalloc = ma_malloc;
                allocationCallbacks.onRealloc = ma_realloc;

                ma_result result = ma_channel_converter_init(&config, &allocationCallbacks, &converter);
                if (result != MA_SUCCESS)
                {
                    ma_channel_converter_uninit(&converter, &allocationCallbacks);
                    amMemory->Free(MemoryPoolKind::Amplimix, channelMap);

                    SoundChunk::DestroyChunk(out);
                    SoundChunk::DestroyChunk(in);

                    CallLogFunc("[ERROR] Cannot process frames. Unable to initialize the channel converter.");

                    return;
                }

                result = ma_channel_converter_process_pcm_frames(
                    &converter, reinterpret_cast<AmInt16Buffer>(out->buffer), reinterpret_cast<AmInt16Buffer>(in->buffer), in->frames);
                if (result != MA_SUCCESS)
                {
                    ma_channel_converter_uninit(&converter, &allocationCallbacks);
                    amMemory->Free(MemoryPoolKind::Amplimix, channelMap);

                    SoundChunk::DestroyChunk(out);
                    SoundChunk::DestroyChunk(in);

                    CallLogFunc("[ERROR] Cannot process frames. Unable to process the channel conversion.");

                    return;
                }

                ma_channel_converter_uninit(&converter, &allocationCallbacks);
                amMemory->Free(MemoryPoolKind::Amplimix, channelMap);
            }
            else
            {
                memcpy(out->buffer, in->buffer, out->size);
            }

            if (sampleRate != baseSampleRate)
            {
                const AmUInt64 inputL = inSamples * reqChannels;
                const AmUInt64 outputL = outSamples * reqChannels;

                auto* inputI16 = reinterpret_cast<AmInt16Buffer>(out->buffer);
                auto* outputI16 = reinterpret_cast<AmInt16Buffer>(out->buffer);

                auto* inputR32 = static_cast<AmReal32Buffer>(amMemory->Malloc(MemoryPoolKind::Amplimix, inputL * sizeof(AmReal32)));
                auto* outputR32 = static_cast<AmReal32Buffer>(amMemory->Malloc(MemoryPoolKind::Amplimix, outputL * sizeof(AmReal32)));

                memset(outputR32, 0, outputL * sizeof(AmReal32));

                for (AmUInt64 i = 0; i < inputL; i++)
                {
                    inputR32[i] = AmInt16ToReal32(inputI16[i]);
                }

                SRC_DATA data;
                data.data_in = inputR32;
                data.data_out = outputR32;
                data.end_of_input = 0;
                data.input_frames = inSamples;
                data.output_frames = outSamples;
                data.src_ratio = static_cast<AmReal64>(sampleRate) / baseSampleRate;

                src_process(layer->sampleRateConverter, &data);

                for (AmUInt64 i = 0; i < outputL; i++)
                {
                    outputI16[i] = AmReal32ToInt16(outputR32[i], true);
                }

                amMemory->Free(MemoryPoolKind::Amplimix, inputR32);
                amMemory->Free(MemoryPoolKind::Amplimix, outputR32);
            }

            switch (layer->snd->format.GetInterleaveType())
            {
            case AM_SAMPLE_INTERLEAVED:
                _pipeline->ProcessInterleaved(
                    reinterpret_cast<AmInt16Buffer>(out->buffer), reinterpret_cast<AmInt16Buffer>(out->buffer), samples, out->size,
                    reqChannels, sampleRate, static_cast<SoundInstance*>(layer->snd->userData));
                break;
            case AM_SAMPLE_NON_INTERLEAVED:
                _pipeline->Process(
                    reinterpret_cast<AmInt16Buffer>(out->buffer), reinterpret_cast<AmInt16Buffer>(out->buffer), samples, out->size,
                    reqChannels, sampleRate, static_cast<SoundInstance*>(layer->snd->userData));
                break;
            default:
                CallLogFunc("[WARNING] A bad sound data interleave type was encountered.\n");
                break;
            }

            /* */ AmUInt64 position = cursor << kAmFixedPointBits;
            const AmUInt64 start = layer->start << kAmFixedPointBits;
            const AmUInt64 end = layer->end << kAmFixedPointBits;

            // regular playback
            for (AmUInt64 i = 0; i < bufferSize; i += reqChannels)
            {
                // check if cursor at end
                if (position >= end)
                {
                    // quit unless looping
                    if (!loop)
                        break;

                    // call the onLoop callback
                    if (OnSoundLooped(this, layer))
                    {
                        src_reset(layer->sampleRateConverter);
                        // wrap around if allowed looping again
                        position = start;
                    }
                    else
                    {
                        // stop playback
                        break;
                    }
                }

                switch (_device.mRequestedOutputChannels)
                {
                case PlaybackOutputChannels::Mono:
                    // lGain is always equal to rGain on mono
                    MixMono(this, i, lGain, out, buffer);
                    break;

                case PlaybackOutputChannels::Stereo:
                    MixStereo(this, i, lGain, rGain, out, buffer);
                    break;

                default:
                    break;
                }

#if defined(AM_SSE_INTRINSICS)
                position += stepFixed * kSimdProcessedFramesCount;
#else
                position += stepFixed;
#endif // AM_SSE_INTRINSICS
            }

            cursor = position >> kAmFixedPointBits;

#if defined(AM_SSE_INTRINSICS)
            cursor = AM_VALUE_ALIGN(cursor, kSimdProcessedFramesCount);
#endif // AM_SSE_INTRINSICS

            // swap back cursor if unchanged
            if (!AMPLIMIX_CSWAP(&layer->cursor, &oldCursor, cursor))
                cursor = oldCursor;

            SoundChunk::DestroyChunk(out);

            // clear flag if PLAY_STATE_FLAG_PLAY and the cursor has reached the end
            if ((flag == PLAY_STATE_FLAG_PLAY) && (cursor == layer->end))
                AMPLIMIX_CSWAP(&layer->flag, &flag, PLAY_STATE_FLAG_MIN);
        }

        SoundChunk::DestroyChunk(in);

        // run callback if reached the end
        if (cursor >= layer->end)
        {
            // We are in the audio thread mutex here
            MixerCommandCallback callback = [=]() -> bool
            {
                // stop playback unless looping
                if (!loop)
                {
                    OnSoundEnded(this, layer);
                }
                else
                {
                    // call the onLoop callback
                    if (OnSoundLooped(this, layer))
                    {
                        // wrap around if allowed looping again
                        AMPLIMIX_CSWAP(&layer->cursor, &layer->end, layer->start);
                    }
                    else
                    {
                        // stop playback
                        OnSoundEnded(this, layer);
                    }
                }

                return true;
            };

            // Postpone call outside the audio thread mutex
            PushCommand({ callback });
        }
    }

    MixerLayer* Mixer::GetLayer(AmUInt32 layer)
    {
        // get layer based on the lowest bits of layer id
        return &_layers[layer & kAmplimixLayersMask];
    }

    bool Mixer::ShouldMix(MixerLayer* layer)
    {
        // load flag value
        PlayStateFlag flag = AMPLIMIX_LOAD(&layer->flag);

        // return if flag is not cleared
        return (flag > PLAY_STATE_FLAG_HALT);
    }

    void Mixer::UpdatePitch(MixerLayer* layer)
    {
        const AmReal32 pitch = AMPLIMIX_LOAD(&layer->pitch);
        const AmReal32 speed = AMPLIMIX_LOAD(&layer->userPlaySpeed);

        if (pitch > 0)
        {
            const AmReal32 playSpeed = pitch * speed;
            AMPLIMIX_STORE(&layer->playSpeed, playSpeed);
            AMPLIMIX_STORE(&layer->sampleRate, playSpeed * _device.mDeviceOutputSampleRate);
        }
        else
        {
            constexpr AmReal32 playSpeed = 0.001f;
            AMPLIMIX_STORE(&layer->playSpeed, playSpeed);
            AMPLIMIX_STORE(&layer->sampleRate, playSpeed * _device.mDeviceOutputSampleRate);
        }
    }

    void Mixer::LockAudioMutex()
    {
        AMPLITUDE_ASSERT(!_insideAudioThreadMutex);

        if (_audioThreadMutex)
        {
            Thread::LockMutex(_audioThreadMutex);
        }

        _insideAudioThreadMutex = true;
    }

    void Mixer::UnlockAudioMutex()
    {
        AMPLITUDE_ASSERT(_insideAudioThreadMutex);

        if (_audioThreadMutex)
        {
            Thread::UnlockMutex(_audioThreadMutex);
        }

        _insideAudioThreadMutex = false;
    }
} // namespace SparkyStudios::Audio::Amplitude
