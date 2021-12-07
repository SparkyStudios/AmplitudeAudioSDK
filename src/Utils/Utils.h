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

#ifndef SS_AMPLITUDE_AUDIO_UTILS_H
#define SS_AMPLITUDE_AUDIO_UTILS_H

#include <SparkyStudios/Audio/Amplitude/Core/Common.h>

#if defined(AM_SSE_INTRINSICS)
#include <Vc/Vc>
#endif // defined(AM_SSE_INTRINSICS)

namespace SparkyStudios::Audio::Amplitude
{
#if defined(AM_SSE_INTRINSICS)
    typedef Vc::int16_v AudioDataUnit;
#else
    typedef AmInt16 AudioDataUnit;
#endif

    typedef AudioDataUnit* AudioBuffer;

    static AM_INLINE(AmInt32) AmFloatToFixedPoint(const AmReal32 x)
    {
        return static_cast<AmInt32>(x * (1 << kAmFixedPointShift));
    }

    static AM_INLINE(AmReal32) AmInt16ToReal32(const AmInt16 x)
    {
        AmReal32 y = static_cast<AmReal32>(x);

#if defined(AM_ACCURATE_CONVERSION)
        // The accurate way.
        y = y + 32768.0f; // -32768..32767 to 0..65535
        y = y * 0.00003051804379339284f; // 0..65535 to 0..2
        y = y - 1; // 0..2 to -1..1
#else
        // The fast way.
        y = y * 0.000030517578125f; // -32768..32767 to -1..0.999969482421875
#endif

        return y;
    }

    static AM_INLINE(AmInt16) AmReal32ToInt16(const AmReal32 x)
    {
        AmReal32 y = AM_CLAMP(x, -1.0f, 1.0f);

#if defined(AM_ACCURATE_CONVERSION)
        // The accurate way.
        y = y + 1; // -1..1 to 0..2
        y = y * 32767.5f; // 0..2 to 0..65535
        y = y - 32768.0f; // 0...65535 to -32768..32767
#else
        // The fast way.
        y = y * 32767.0f; // -1..1 to -32767..32767
#endif

        return static_cast<AmInt16>(y);
    }

    static AM_INLINE(AmReal32) CatmullRom(const AmReal32 t, const AmReal32 p0, const AmReal32 p1, const AmReal32 p2, const AmReal32 p3)
    {
        // clang-format off
        return 0.5f * (
            (2 * p1) +
            (-p0 + p2) * t +
            (2 * p0 - 5 * p1 + 4 * p2 - p3) * t * t +
            (-p0 + 3 * p1 - 3 * p2 + p3) * t * t * t
        );
        // clang-format on
    }
} // namespace SparkyStudios::Audio::Amplitude

#endif // SS_AMPLITUDE_AUDIO_UTILS_H