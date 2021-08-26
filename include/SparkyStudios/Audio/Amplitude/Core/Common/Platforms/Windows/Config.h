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

#ifndef SS_AMPLITUDE_AUDIO_WINDOWS_CONFIG_H
#define SS_AMPLITUDE_AUDIO_WINDOWS_CONFIG_H

#include <sstream>

// Enable Windows Compilation
#define AM_WINDOWS_VERSION

// Detect the platform CPU type
#if defined(_M_IX86)
#define AM_CPU_X86
#elif defined(_M_AMD64)
#define AM_CPU_X86_64
#elif defined(_M_ARM)
#define AM_CPU_ARM
#define AM_CPU_ARM_NEON
#elif defined(_M_ARM64)
#define AM_CPU_ARM_64
#define AM_CPU_ARM_NEON
#endif

// Function inline
#define AM_INLINE __forceinline
#define AM_NO_INLINE __declspec(noinline)

// Alignment required for SIMD data processing
#define AM_SIMD_ALIGNMENT 16
#define AM_ALIGN_SIMD(_declaration_) AM_ALIGN(_declaration_, AM_SIMD_ALIGNMENT)
#define AM_BUFFER_ALIGNMENT AM_SIMD_ALIGNMENT

// Windows platforms support wchar_t
#define AM_WCHAR_SUPPORTED

// Defines the format used to print AmOsString text
#define AM_OS_CHAR_FMT "%ls"

// Macro used to convert a string literal to an AmOsString string at compile-time
#define AM_OS_STRING(s) L##s

AM_INLINE std::wstring am_string_widen(const std::string& str)
{
    std::wostringstream wstm;
    const auto& ctfacet = std::use_facet<std::ctype<wchar_t>>(wstm.getloc());
    for (char i : str)
        wstm << ctfacet.widen(i);
    return wstm.str();
}

AM_INLINE std::string am_wstring_narrow(const std::wstring& str)
{
    std::ostringstream stm;
    const auto& ctfacet = std::use_facet<std::ctype<wchar_t>>(stm.getloc());
    for (wchar_t i : str)
        stm << ctfacet.narrow(i, 0);
    return stm.str();
}

// Conversion between OS strings and default strings
#define AM_OS_STRING_TO_STRING(s) am_wstring_narrow(s).c_str()
#define AM_STRING_TO_OS_STRING(s) am_string_widen(s).c_str()

// AMPLITUDE_ASSERT Config
#ifdef AMPLITUDE_NO_ASSERTS
#define AMPLITUDE_ASSERT(x)
#else
#ifdef _MSC_VER
#include <cstdio> // for sprintf in asserts
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h> // only needed for OutputDebugStringA, should be solved somehow.
#define AMPLITUDE_ASSERT(x)                                                                                                                \
    if (!(x))                                                                                                                              \
    {                                                                                                                                      \
        char temp[200];                                                                                                                    \
        sprintf(temp, "%s(%d): assert(%s) failed.\n", __FILE__, __LINE__, #x);                                                             \
        OutputDebugStringA(temp);                                                                                                          \
        __debugbreak();                                                                                                                    \
    }
#else
#include <cassert> // assert
#define AMPLITUDE_ASSERT(x) assert(x)
#endif
#endif

#endif // SS_AMPLITUDE_AUDIO_WINDOWS_CONFIG_H