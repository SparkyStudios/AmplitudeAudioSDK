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

#ifndef SS_AMPLITUDE_AUDIO_VORBIS_CODEC_PLUGIN_H
#define SS_AMPLITUDE_AUDIO_VORBIS_CODEC_PLUGIN_H

#include <SparkyStudios/Audio/Amplitude/Amplitude.h>

using namespace SparkyStudios::Audio::Amplitude;

AM_API_PRIVATE Engine* GetEngine();

AM_API_PRIVATE MemoryManager* GetMemoryManager();

#define amEngine GetEngine()
#define amMemory GetMemoryManager()

#endif // SS_AMPLITUDE_AUDIO_VORBIS_CODEC_PLUGIN_H
