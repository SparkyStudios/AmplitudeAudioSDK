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

#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

#include <Core/Codecs/MP3/Codec.h>

namespace SparkyStudios::Audio::Amplitude::Codecs
{
    bool MP3Codec::MP3Decoder::Open(AmOsString filePath)
    {
        if (!m_codec->CanHandleFile(filePath))
        {
            CallLogFunc("The MP3 codec cannot handle the file: '%s'\n", filePath);
            return false;
        }

#if defined(AM_WCHAR_SUPPORTED)
        if (drmp3_init_file_w(&_mp3, filePath, nullptr) == DRMP3_FALSE)
#else
        if (drmp3_init_file(&_mp3, filePath, nullptr) == DRMP3_FALSE)
#endif
        {
            CallLogFunc("[ERROR] Cannot load the MP3 file: '%s'\n.", filePath);
            return false;
        }

        drmp3_uint64 framesCount = drmp3_get_pcm_frame_count(&_mp3);
        if (framesCount == DRMP3_FALSE)
        {
            CallLogFunc("[ERROR] Cannot load the MP3 file: '%s'\n.", filePath);
            return false;
        }

        m_format.SetAll(
            _mp3.sampleRate, _mp3.channels, 0, framesCount, _mp3.channels * sizeof(float),
            AM_SAMPLE_FORMAT_FLOAT, // This codec always read frames as float32 values
            AM_SAMPLE_INTERLEAVED // dr_mp3 always read interleaved frames
        );

        _initialized = true;

        return true;
    }

    bool MP3Codec::MP3Decoder::Close()
    {
        if (_initialized)
        {
            m_format = SoundFormat();
            _initialized = false;
            drmp3_uninit(&_mp3);
        }

        // true because it is already closed
        return true;
    }

    AmUInt64 MP3Codec::MP3Decoder::Load(AmReal32Buffer out)
    {
        if (!_initialized)
        {
            return 0;
        }

        if (drmp3_seek_to_pcm_frame(&_mp3, 0) != DRMP3_TRUE)
        {
            return 0;
        }

        return drmp3_read_pcm_frames_f32(&_mp3, m_format.GetFramesCount(), out);
    }

    AmUInt64 MP3Codec::MP3Decoder::Stream(AmReal32Buffer out, AmUInt64 offset, AmUInt64 length)
    {
        if (!_initialized)
        {
            return 0;
        }

        if (!Seek(offset))
        {
            return 0;
        }

        return drmp3_read_pcm_frames_f32(&_mp3, length, out);
    }

    bool MP3Codec::MP3Decoder::Seek(AmUInt64 offset)
    {
        return drmp3_seek_to_pcm_frame(&_mp3, offset) == DRMP3_TRUE;
    }

    bool MP3Codec::MP3Encoder::Open(AmOsString filePath)
    {
        _initialized = true;
        return false;
    }

    bool MP3Codec::MP3Encoder::Close()
    {
        if (_initialized)
        {
            return true;
        }

        return true;
    }

    AmUInt64 MP3Codec::MP3Encoder::Write(const float* in, AmUInt64 offset, AmUInt64 length)
    {
        return 0;
    }

    Codec::Decoder* MP3Codec::CreateDecoder() const
    {
        return new MP3Decoder(this);
    }

    Codec::Encoder* MP3Codec::CreateEncoder() const
    {
        return new MP3Encoder(this);
    }

    bool MP3Codec::CanHandleFile(AmOsString filePath) const
    {
        // TODO: Maybe check by extension instead?

        drmp3 dummy;
#if defined(AM_WCHAR_SUPPORTED)
        bool can = drmp3_init_file_w(&dummy, filePath, nullptr) == DRMP3_TRUE;
#else
        bool can = drmp3_init_file(&dummy, filePath, nullptr) == DRMP3_TRUE;
#endif

        if (can)
        {
            drmp3_uninit(&dummy);
        }

        return can;
    }
} // namespace SparkyStudios::Audio::Amplitude::Codecs