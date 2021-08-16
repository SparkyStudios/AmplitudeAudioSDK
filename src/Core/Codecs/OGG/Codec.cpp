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

#include "stb_vorbis.h"

#include <Core/Codecs/OGG/Codec.h>

namespace SparkyStudios::Audio::Amplitude::Codecs
{
    bool OGGCodec::OGGDecoder::Open(AmString filePath)
    {
        if (!m_codec->CanHandleFile(filePath))
        {
            CallLogFunc("The OGG codec cannot handle the file: '%s'\n", filePath);
            return false;
        }

        if (_ogg = stb_vorbis_open_filename(filePath, nullptr, nullptr); _ogg == nullptr)
        {
            CallLogFunc("Cannot load the OGG file: '%s'\n.", filePath);
            return false;
        }

        stb_vorbis_info info = stb_vorbis_get_info(_ogg);
        AmUInt32 framesCount = stb_vorbis_stream_length_in_samples(_ogg);

        m_format.SetAll(
            info.sample_rate, info.channels, info.max_frame_size, framesCount, info.channels * info.max_frame_size >> 3,
            AM_SAMPLE_FORMAT_FLOAT, // This codec always read frames as float32 values
            AM_SAMPLE_INTERLEAVED // stb_vorbis always read interleaved frames
        );

        _initialized = true;

        return true;
    }

    bool OGGCodec::OGGDecoder::Close()
    {
        if (_initialized)
        {
            m_format = SoundFormat();
            _initialized = false;
            stb_vorbis_close(_ogg);
            return true;
        }

        // true because it is already closed
        return true;
    }

    AmUInt64 OGGCodec::OGGDecoder::Load(AmFloat32Buffer out)
    {
        if (!_initialized)
        {
            return 0;
        }

        if (stb_vorbis_seek_start(_ogg) == 0)
        {
            return 0;
        }

        return stb_vorbis_get_samples_float_interleaved(_ogg, m_format.GetNumChannels(), out, m_format.GetFramesCount());
    }

    AmUInt64 OGGCodec::OGGDecoder::Stream(AmFloat32Buffer out, AmUInt64 offset, AmUInt64 length)
    {
        if (!_initialized)
        {
            return 0;
        }

        if (!Seek(offset))
        {
            return 0;
        }

        return stb_vorbis_get_samples_float_interleaved(_ogg, m_format.GetNumChannels(), out, length);
    }

    bool OGGCodec::OGGDecoder::Seek(AmUInt64 offset)
    {
        return stb_vorbis_seek_frame(_ogg, offset) != 0;
    }

    bool OGGCodec::OGGEncoder::Open(AmString filePath)
    {
        _initialized = true;
        return false;
    }

    bool OGGCodec::OGGEncoder::Close()
    {
        if (_initialized)
        {
            return true;
        }

        return true;
    }

    AmUInt64 OGGCodec::OGGEncoder::Write(const float* in, AmUInt64 offset, AmUInt64 length)
    {
        return 0;
    }

    Codec::Decoder* OGGCodec::CreateDecoder() const
    {
        return new OGGDecoder(this);
    }

    Codec::Encoder* OGGCodec::CreateEncoder() const
    {
        return new OGGEncoder(this);
    }

    bool OGGCodec::CanHandleFile(AmString filePath) const
    {
        // TODO: Maybe check by extension instead?

        if (stb_vorbis* h = stb_vorbis_open_filename(filePath, nullptr, nullptr); h != nullptr)
        {
            stb_vorbis_close(h);
            return true;
        }

        return false;
    }
} // namespace SparkyStudios::Audio::Amplitude::Codecs