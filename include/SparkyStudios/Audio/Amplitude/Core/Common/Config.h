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

#ifndef SS_AMPLITUDE_AUDIO_CONFIG_H
#define SS_AMPLITUDE_AUDIO_CONFIG_H

#ifndef AM_ALIGN
#if defined(_MSC_VER)
#define AM_ALIGN(_declaration_, _alignment_) __declspec(align(_alignment_)) _declaration_
#else
#define AK_ALIGN(_declaration_, _alignment_) _declaration_ __attribute__((aligned(_alignment_)))
#endif
#endif

#if defined(_WIN32) || defined(_WIN64) || defined(WINAPI_FAMILY)
#include <SparkyStudios/Audio/Amplitude/Core/Common/Platforms/Windows/Config.h>
#elif defined(__ANDROID__)
#include <SparkyStudios/Audio/Amplitude/Core/Common/Platforms/Android/Config.h>
#elif defined(__linux__)
#include <SparkyStudios/Audio/Amplitude/Core/Common/Platforms/Linux/Config.h>
#endif

#if !defined(AMPLITUDE_DISABLE_SIMD)
#if defined(AM_CPU_X86) || defined(AM_CPU_X86_64)
#define AM_SSE_INTRINSICS
#endif
#endif

// Define the value of Pi if the platform doesn't do that
#ifndef M_PI
#define M_PI 3.14159265358979323846264f // from CRC
#endif

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
// Configuration defines

// Maximum number of filters per stream
#define AM_FILTERS_PER_STREAM 8

// 1) Mono, 2) Stereo, 4) Quad, 6) 5.1, 8) 7.1
#define AM_MAX_CHANNELS 8

#endif // SS_AMPLITUDE_AUDIO_CONFIG_H