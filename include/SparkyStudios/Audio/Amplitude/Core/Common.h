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

#ifndef SS_AMPLITUDE_AUDIO_COMMON_H
#define SS_AMPLITUDE_AUDIO_COMMON_H

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
// Standard Library

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
// Amplitude

#include <SparkyStudios/Audio/Amplitude/Core/Common/Config.h>
#include <SparkyStudios/Audio/Amplitude/Core/Common/Types.h>

#include <SparkyStudios/Audio/Amplitude/Core/Common/Constants.h>

#include <SparkyStudios/Audio/Amplitude/Math/HandmadeMath.h>

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
// Common defines

// Define an invalid object handle
#define AM_INVALID_HANDLE 0

// Check for handle validity
#define AM_IS_VALID_HANDLE(handle) ((handle) != AM_INVALID_HANDLE)

// Check for channels value validity
#define AM_IS_VALID_CHANNELS_VALUE(channels) ((channels) == 1 || (channels) == 2 || (channels) == 4 || (channels) == 6 || (channels) == 8)

// Clamps a value between a and b
#define AM_CLAMP(v, a, b) (((v) < (a)) ? (a) : ((v) > (b)) ? (b) : (v))

// Checks if a value is between a and b
#define AM_BETWEEN(v, a, b) ((v) >= AM_MIN(a, b) && (v) <= AM_MAX(a, b))

/**
 * @brief Declare a callback function type
 *
 * @param _type_ Return type of the function
 * @param _name_ Name of the function
 *
 * @remarks This must be followed by the parentheses containing the function arguments declaration
 */
#define AM_CALLBACK(_type_, _name_) typedef _type_(AM_CALL_POLICY* _name_)

/**
 * @brief The minimum value for an audio sample.
 */
#define AM_AUDIO_SAMPLE_MIN (-1.0f)

/**
 * @brief The maximum value for an audio sample.
 */
#define AM_AUDIO_SAMPLE_MAX (+1.0f)

/**
 * @brief Clamps an audio sample value between AM_AUDIO_SAMPLE_MIN and AM_AUDIO_SAMPLE_MAX.
 */
#define AM_CLAMP_AUDIO_SAMPLE(v) (((v) <= -1.65f) ? -0.9862875f : ((v) >= 1.65f) ? 0.9862875f : (0.87f * (v)-0.1f * (v) * (v) * (v)))

/**
 * @brief Helps to avoid compiler warnings about unused values.
 *
 * @param x The statement where the return value is not used.
 */
#define AM_UNUSED(x) ((void)(x))

/**
 * @brief Turn X into a string literal.
 *
 * @param x The value to transform into a string literal
 */
#define AM_STRING_EXPAND(X) #X
#define AM_TO_STRING(X) AM_STRING_EXPAND(X)

namespace SparkyStudios::Audio::Amplitude
{
    /**
     * @brief Class that handles aligned allocations to support vectorized operations.
     */
    class AM_API_PUBLIC AmAlignedReal32Buffer
    {
    public:
        AmAlignedReal32Buffer();
        ~AmAlignedReal32Buffer();

        /**
         * @brief Allocates and align buffer.
         *
         * @param size The buffer size.
         * @return the result of the allocation.
         */
        AmResult Init(AmUInt32 size);

        /**
         * @brief Clears all data.
         */
        void Clear() const;

        /**
         * @brief Releases the allocated buffer.
         */
        void Release();

        /**
         * @brief Gets the size of the buffer.
         *
         * @return AmUInt32
         */
        [[nodiscard]] AM_INLINE(AmUInt32) GetSize() const
        {
            return m_floats;
        }

        /**
         * @brief Gets the current aligned pointer.
         *
         * @return AmFloat32Buffer
         */
        [[nodiscard]] AM_INLINE(AmReal32Buffer) GetBuffer() const
        {
            return m_data;
        }

        /**
         * @brief Gets the raw allocated pointer.
         *
         * @return AmUInt8Buffer
         */
        [[nodiscard]] AM_INLINE(AmUInt8Buffer) GetPointer() const
        {
            return m_basePtr;
        }

        /**
         * @brief Copies data from another buffer.
         * @param other The other buffer to copy data from.
         */
        void CopyFrom(const AmAlignedReal32Buffer& other) const;

        /**
         * @brief Resizes the buffer to the specified size.
         *
         * @param size The new size of the buffer.
         */
        void Resize(AmUInt32 size);

        /**
         * @brief Swaps two buffers.
         *
         * @param a The first buffer.
         * @param b The second buffer.
         */
        static void Swap(AmAlignedReal32Buffer& a, AmAlignedReal32Buffer& b);

        AmReal32& operator[](AmSize index)
        {
            AMPLITUDE_ASSERT(m_data && index < m_floats);
            return m_data[index];
        }

        const AmReal32& operator[](size_t index) const
        {
            AMPLITUDE_ASSERT(m_data && index < m_floats);
            return m_data[index];
        }

    private:
        AmReal32Buffer m_data; // aligned pointer
        AmUInt8Buffer m_basePtr; // raw allocated pointer (for delete)
        AmUInt32 m_floats; // size of buffer (w/out padding)
    };

    /**
     * @brief Lightweight class that handles small aligned buffer to support vectorized operations.
     */
    class AM_API_PUBLIC AmTinyAlignedReal32Buffer
    {
    public:
        /**
         * @brief Construct a new TinyAlignedReal32Buffer object by allocating an aligned
         * buffer with AM_SIMD_ALIGNMENT float values.
         */
        AmTinyAlignedReal32Buffer();

        /**
         * @brief Gets the raw allocated pointer.
         *
         * @return AmReal32Buffer
         */
        [[nodiscard]] AM_INLINE(AmReal32Buffer) GetBuffer() const
        {
            return m_data;
        }

    private:
        AmReal32Buffer m_data; // aligned pointer
        AmUInt8 m_actualData[sizeof(AmReal32) * AM_SIMD_ALIGNMENT + AM_SIMD_ALIGNMENT]{};
    };

    /**
     * @brief Enumerates the list of possible errors encountered by the library.
     */
    enum AM_ERROR : AmUInt8
    {
        AM_ERROR_NO_ERROR = 0, // No error
        AM_ERROR_INVALID_PARAMETER = 1, // Some parameter is invalid
        AM_ERROR_FILE_NOT_FOUND = 2, // File not found
        AM_ERROR_FILE_LOAD_FAILED = 3, // File found, but could not be loaded
        AM_ERROR_DLL_NOT_FOUND = 4, // DLL not found, or wrong DLL
        AM_ERROR_OUT_OF_MEMORY = 5, // Out of memory
        AM_ERROR_NOT_IMPLEMENTED = 6, // Feature not implemented
        AM_ERROR_UNKNOWN = 7 // Other error
    };

    /**
     * @brief Enumerates the list of possible sample formats handled by Amplitude.
     */
    enum AM_SAMPLE_FORMAT : AmUInt8
    {
        // floating point (float32)
        AM_SAMPLE_FORMAT_FLOAT,
        // integer (int16)
        AM_SAMPLE_FORMAT_INT,
        // An unknown format
        AM_SAMPLE_FORMAT_UNKNOWN,
    };

    /**
     * @brief Enumerates the list of states in a fader.
     */
    enum AM_FADER_STATE : AmInt8
    {
        AM_FADER_STATE_STOPPED = -1,
        AM_FADER_STATE_DISABLED = 0,
        AM_FADER_STATE_ACTIVE = 1,
    };

    /**
     * @brief Describe the format of an audio sample.
     *
     * This data structure is mainly filled by a Codec
     * during the initialization time.
     */
    struct AM_API_PUBLIC SoundFormat
    {
    public:
        void SetAll(
            AmUInt32 sampleRate,
            AmUInt16 numChannels,
            AmUInt32 bitsPerSample,
            AmUInt64 framesCount,
            AmUInt32 frameSize,
            AM_SAMPLE_FORMAT sampleType);

        /**
         * @brief Get the sample rate.
         *
         * @return AmUInt32
         */
        [[nodiscard]] AM_INLINE(AmUInt32) GetSampleRate() const
        {
            return _sampleRate;
        }

        /**
         * @brief Get the number of channels.
         *
         * @return AmUInt16
         */
        [[nodiscard]] AM_INLINE(AmUInt16) GetNumChannels() const
        {
            return _numChannels;
        }

        /**
         * @brief Get the bits per sample.
         *
         * @return AmUInt32
         */
        [[nodiscard]] AM_INLINE(AmUInt32) GetBitsPerSample() const
        {
            return _bitsPerSample;
        }

        /**
         * @brief Get the number of frames.
         *
         * @return AmUInt64
         */
        [[nodiscard]] AM_INLINE(AmUInt64) GetFramesCount() const
        {
            return _framesCount;
        }

        /**
         * @brief Get the frame size.
         *
         * @return AmUInt32
         */
        [[nodiscard]] AM_INLINE(AmUInt32) GetFrameSize() const
        {
            return _frameSize;
        }

        /**
         * @brief Get the sample type.
         *
         * @return AM_SAMPLE_FORMAT
         */
        [[nodiscard]] AM_INLINE(AM_SAMPLE_FORMAT) GetSampleType() const
        {
            return _sampleType;
        }

    private:
        AmUInt32 _sampleRate = 0;
        AmUInt16 _numChannels = 0;
        AmUInt32 _bitsPerSample = 0;
        AmUInt64 _framesCount = 0;
        AmUInt32 _frameSize = 0;
        AM_SAMPLE_FORMAT _sampleType = AM_SAMPLE_FORMAT_FLOAT;
    };
} // namespace SparkyStudios::Audio::Amplitude

#endif // SS_AMPLITUDE_AUDIO_COMMON_H
