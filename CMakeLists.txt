# Copyright (c) 2021-present Sparky Studios. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.20)

cmake_policy(SET CMP0048 NEW)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(DetectAmplitudeVersion)

option(BUILD_SAMPLES "Build samples" OFF)
option(UNIT_TESTS "Enable Unit Testing" OFF)

if(BUILD_SAMPLES)
    list(APPEND VCPKG_MANIFEST_FEATURES "samples")
endif()

if(UNIT_TESTS)
    list(APPEND VCPKG_MANIFEST_FEATURES "tests")
endif()

project(Amplitude VERSION ${DETECTED_AM_VERSION})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_DEBUG_POSTFIX "_d")

set(BIN_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(OBJ_DIR ${CMAKE_CURRENT_BINARY_DIR}/obj)

if(MSVC OR WIN32)
    set(RUNTIME_OUTPUT_DIRECTORY "win")
elseif(APPLE)
    set(RUNTIME_OUTPUT_DIRECTORY "osx")
else()
    set(RUNTIME_OUTPUT_DIRECTORY "linux")
endif()

set(AM_LIB_DESTINATION "lib/${RUNTIME_OUTPUT_DIRECTORY}")
set(AM_INC_DESTINATION "include")
set(AM_SHM_DESTINATION "schemas")
set(AM_BIN_DESTINATION "bin/${RUNTIME_OUTPUT_DIRECTORY}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${AM_BIN_DESTINATION}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${AM_LIB_DESTINATION}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${AM_BIN_DESTINATION}")

set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_LIST_DIR}/sdk")

if(UNIX)
    find_package(Threads REQUIRED)
endif()

# Setup FFT and Accelerate Framework
# ------------------------------------------------------------------------------
if(APPLE)
    find_library(ACCELERATE_FRAMEWORK Accelerate)
endif()

# ------------------------------------------------------------------------------

# Setup flatbuffers
# ------------------------------------------------------------------------------
find_package(flatbuffers CONFIG REQUIRED)

# ------------------------------------------------------------------------------

# Setup libsamplerate
# ------------------------------------------------------------------------------
find_package(SampleRate CONFIG REQUIRED)

# ------------------------------------------------------------------------------

# Setup xsimd
# ------------------------------------------------------------------------------
find_package(xsimd CONFIG REQUIRED)

# ------------------------------------------------------------------------------

# Setup mimalloc
# ------------------------------------------------------------------------------
find_package(mimalloc CONFIG REQUIRED)

# Check what instruction sets the current host supports.
include(DetectCPUArchitecture)
am_buildsystem_get_runnable_archs(NATIVE_ARCHS)

foreach(ARCH ${NATIVE_ARCHS})
    am_buildsystem_get_arch_info(CXX_FLAGS DEFINES_LIST SUFFIX ${ARCH})

    separate_arguments(CXX_FLAGS)
    add_compile_options(${CXX_FLAGS})
endforeach()

# ------------------------------------------------------------------------------

# Generate source files for all FlatBuffers schema files under the src
# directory.
set(AM_FLATBUFFERS_GENERATED_INCLUDES_DIR ${OBJ_DIR}/include/SparkyStudios/Audio/Amplitude)
file(GLOB_RECURSE FLATBUFFERS_SCHEMAS ${CMAKE_CURRENT_SOURCE_DIR}/schemas/*.fbs)

# Generate rules to build the set of output files from the set of input
# schema files.
build_flatbuffers("${FLATBUFFERS_SCHEMAS}"
    ""
    generated_includes
    ""
    ${AM_FLATBUFFERS_GENERATED_INCLUDES_DIR}
    ""
    ""
)

# Setup project source files.
set(SA_SOURCE
    include/SparkyStudios/Audio/Amplitude/Convolution/Convolver.h
    include/SparkyStudios/Audio/Amplitude/Convolution/TwoStageConvolver.h
    include/SparkyStudios/Audio/Amplitude/Core/Common/Platforms/Android/Config.h
    include/SparkyStudios/Audio/Amplitude/Core/Common/Platforms/Android/Types.h
    include/SparkyStudios/Audio/Amplitude/Core/Common/Platforms/Linux/Config.h
    include/SparkyStudios/Audio/Amplitude/Core/Common/Platforms/Linux/Types.h
    include/SparkyStudios/Audio/Amplitude/Core/Common/Platforms/UNIX/Config.h
    include/SparkyStudios/Audio/Amplitude/Core/Common/Platforms/UNIX/Types.h
    include/SparkyStudios/Audio/Amplitude/Core/Common/Platforms/Windows/Config.h
    include/SparkyStudios/Audio/Amplitude/Core/Common/Platforms/Windows/Types.h
    include/SparkyStudios/Audio/Amplitude/Core/Common/Config.h
    include/SparkyStudios/Audio/Amplitude/Core/Common/Constants.h
    include/SparkyStudios/Audio/Amplitude/Core/Common/SIMD.h
    include/SparkyStudios/Audio/Amplitude/Core/Common/Types.h
    include/SparkyStudios/Audio/Amplitude/Core/Asset.h
    include/SparkyStudios/Audio/Amplitude/Core/Bus.h
    include/SparkyStudios/Audio/Amplitude/Core/Channel.h
    include/SparkyStudios/Audio/Amplitude/Core/Codec.h
    include/SparkyStudios/Audio/Amplitude/Core/Common.h
    include/SparkyStudios/Audio/Amplitude/Core/Device.h
    include/SparkyStudios/Audio/Amplitude/Core/Driver.h
    include/SparkyStudios/Audio/Amplitude/Core/Engine.h
    include/SparkyStudios/Audio/Amplitude/Core/Entity.h
    include/SparkyStudios/Audio/Amplitude/Core/Environment.h
    include/SparkyStudios/Audio/Amplitude/Core/Event.h
    include/SparkyStudios/Audio/Amplitude/Core/Listener.h
    include/SparkyStudios/Audio/Amplitude/Core/Log.h
    include/SparkyStudios/Audio/Amplitude/Core/Memory.h
    include/SparkyStudios/Audio/Amplitude/Core/RefCounter.h
    include/SparkyStudios/Audio/Amplitude/Core/Thread.h
    include/SparkyStudios/Audio/Amplitude/Core/Version.h
    include/SparkyStudios/Audio/Amplitude/IO/File.h
    include/SparkyStudios/Audio/Amplitude/IO/FileSystem.h
    include/SparkyStudios/Audio/Amplitude/Math/Curve.h
    include/SparkyStudios/Audio/Amplitude/Math/FFT.h
    include/SparkyStudios/Audio/Amplitude/Math/HandmadeMath.h
    include/SparkyStudios/Audio/Amplitude/Math/Shape.h
    include/SparkyStudios/Audio/Amplitude/Math/SplitComplex.h
    include/SparkyStudios/Audio/Amplitude/Math/Utils.h
    include/SparkyStudios/Audio/Amplitude/Mixer/Resampler.h
    include/SparkyStudios/Audio/Amplitude/Mixer/SoundProcessor.h
    include/SparkyStudios/Audio/Amplitude/Sound/Attenuation.h
    include/SparkyStudios/Audio/Amplitude/Sound/Collection.h
    include/SparkyStudios/Audio/Amplitude/Sound/Effect.h
    include/SparkyStudios/Audio/Amplitude/Sound/Fader.h
    include/SparkyStudios/Audio/Amplitude/Sound/Filter.h
    include/SparkyStudios/Audio/Amplitude/Sound/Rtpc.h
    include/SparkyStudios/Audio/Amplitude/Sound/Scheduler.h
    include/SparkyStudios/Audio/Amplitude/Sound/Sound.h
    include/SparkyStudios/Audio/Amplitude/Sound/SoundBank.h
    include/SparkyStudios/Audio/Amplitude/Sound/SoundObject.h
    include/SparkyStudios/Audio/Amplitude/Sound/Switch.h
    include/SparkyStudios/Audio/Amplitude/Sound/SwitchContainer.h
    include/SparkyStudios/Audio/Amplitude/Amplitude.h

    src/Convolution/Convolver.cpp
    src/Convolution/TwoStageConvolver.cpp

    src/Core/Codecs/AMS/Codec.cpp
    src/Core/Codecs/AMS/Codec.h

    src/Core/Codecs/MP3/Codec.cpp
    src/Core/Codecs/MP3/Codec.h
    src/Core/Codecs/MP3/dr_mp3.h

    src/Core/Codecs/WAV/Codec.cpp
    src/Core/Codecs/WAV/Codec.h
    src/Core/Codecs/WAV/dr_wav.h

    src/Core/Drivers/MiniAudio/Driver.cpp
    src/Core/Drivers/MiniAudio/Driver.h

    src/Core/Drivers/Null/Driver.cpp
    src/Core/Drivers/Null/Driver.h

    src/Core/Asset.cpp
    src/Core/Bus.cpp
    src/Core/BusInternalState.cpp
    src/Core/BusInternalState.h
    src/Core/Channel.cpp
    src/Core/ChannelInternalState.cpp
    src/Core/ChannelInternalState.h
    src/Core/Codec.cpp
    src/Core/Common.cpp
    src/Core/DefaultPlugins.h
    src/Core/Device.cpp
    src/Core/Driver.cpp
    src/Core/Engine.cpp
    src/Core/EngineInternalState.h
    src/Core/Entity.cpp
    src/Core/EntityInternalState.cpp
    src/Core/EntityInternalState.h
    src/Core/Environment.cpp
    src/Core/EnvironmentInternalState.cpp
    src/Core/EnvironmentInternalState.h
    src/Core/Event.cpp
    src/Core/ListenerInternalState.cpp
    src/Core/ListenerInternalState.h
    src/Core/Listener.cpp
    src/Core/Log.cpp
    src/Core/Memory.cpp
    src/Core/Thread.cpp
    src/Core/Version.cpp

    src/IO/File.cpp
    src/IO/FileSystem.cpp

    src/Math/Curve.cpp
    src/Math/FFT.cpp
    src/Math/Shape.cpp
    src/Math/SplitComplex.cpp

    src/Mixer/Resamplers/R8BrainResampler.h
    src/Mixer/Resamplers/LibsamplerateResampler.h
    src/Mixer/SoundProcessors/ClipProcessor.h
    src/Mixer/SoundProcessors/EffectProcessor.h
    src/Mixer/SoundProcessors/EnvironmentProcessor.h
    src/Mixer/SoundProcessors/ObstructionProcessor.h
    src/Mixer/SoundProcessors/OcclusionProcessor.h
    src/Mixer/SoundProcessors/PassThroughProcessor.h
    src/Mixer/SoundProcessors/SilenceProcessor.h
    src/Mixer/Mixer.cpp
    src/Mixer/Mixer.h
    src/Mixer/ProcessorPipeline.cpp
    src/Mixer/ProcessorPipeline.h
    src/Mixer/RealChannel.cpp
    src/Mixer/RealChannel.h
    src/Mixer/Resampler.cpp
    src/Mixer/SoundData.cpp
    src/Mixer/SoundData.h
    src/Mixer/SoundProcessor.cpp

    src/Sound/Faders/ConstantFader.h
    src/Sound/Faders/EaseFader.h
    src/Sound/Faders/EaseInFader.h
    src/Sound/Faders/EaseInOutFader.h
    src/Sound/Faders/EaseOutFader.h
    src/Sound/Faders/ExponentialFader.h
    src/Sound/Faders/LinearFader.h
    src/Sound/Faders/SCurveFader.h
    src/Sound/Filters/BassBoostFilter.cpp
    src/Sound/Filters/BassBoostFilter.h
    src/Sound/Filters/BiquadResonantFilter.cpp
    src/Sound/Filters/BiquadResonantFilter.h
    src/Sound/Filters/DCRemovalFilter.cpp
    src/Sound/Filters/DCRemovalFilter.h
    src/Sound/Filters/DelayFilter.cpp
    src/Sound/Filters/DelayFilter.h
    src/Sound/Filters/EqualizerFilter.cpp
    src/Sound/Filters/EqualizerFilter.h
    src/Sound/Filters/FFTFilter.cpp
    src/Sound/Filters/FFTFilter.h
    src/Sound/Filters/FlangerFilter.cpp
    src/Sound/Filters/FlangerFilter.h
    src/Sound/Filters/FreeverbFilter.cpp
    src/Sound/Filters/FreeverbFilter.h
    src/Sound/Filters/LofiFilter.cpp
    src/Sound/Filters/LofiFilter.h
    src/Sound/Filters/RobotizeFilter.cpp
    src/Sound/Filters/RobotizeFilter.h
    src/Sound/Filters/WaveShaperFilter.cpp
    src/Sound/Filters/WaveShaperFilter.h
    src/Sound/Schedulers/RandomScheduler.cpp
    src/Sound/Schedulers/RandomScheduler.h
    src/Sound/Schedulers/SequenceScheduler.cpp
    src/Sound/Schedulers/SequenceScheduler.h
    src/Sound/Attenuation.cpp
    src/Sound/AttenuationShapes.cpp
    src/Sound/AttenuationShapes.h
    src/Sound/Collection.cpp
    src/Sound/Effect.cpp
    src/Sound/Fader.cpp
    src/Sound/Filter.cpp
    src/Sound/RefCounter.cpp
    src/Sound/Rtpc.cpp
    src/Sound/Sound.cpp
    src/Sound/SoundBank.cpp
    src/Sound/SoundObject.cpp
    src/Sound/Switch.cpp
    src/Sound/SwitchContainer.cpp

    src/Utils/Audio/Compression/ADPCM/ADPCM.cpp
    src/Utils/Audio/Compression/ADPCM/ADPCM.h
    src/Utils/Audio/FFT/AudioFFT.cpp
    src/Utils/Audio/FFT/AudioFFT.h
    src/Utils/Audio/Resampling/CDSPBlockConvolver.h
    src/Utils/Audio/Resampling/CDSPFIRFilter.h
    src/Utils/Audio/Resampling/CDSPFracInterpolator.h
    src/Utils/Audio/Resampling/CDSPHBDownsampler.h
    src/Utils/Audio/Resampling/CDSPHBUpsampler.h
    src/Utils/Audio/Resampling/CDSPProcessor.h
    src/Utils/Audio/Resampling/CDSPRealFFT.h
    src/Utils/Audio/Resampling/CDSPResampler.h
    src/Utils/Audio/Resampling/CDSPSincFilterGen.h
    src/Utils/Audio/Resampling/r8bbase.cpp
    src/Utils/Audio/Resampling/r8bbase.h
    src/Utils/Audio/Resampling/r8bconf.h
    src/Utils/Audio/Resampling/r8butil.h
    src/Utils/Freeverb/AllPass.cpp
    src/Utils/Freeverb/AllPass.h
    src/Utils/Freeverb/Comb.cpp
    src/Utils/Freeverb/Comb.h
    src/Utils/Freeverb/denormals.h
    src/Utils/Freeverb/ReverbModel.cpp
    src/Utils/Freeverb/ReverbModel.h
    src/Utils/Freeverb/tuning.h
    src/Utils/miniaudio/miniaudio.cpp
    src/Utils/miniaudio/miniaudio.h
    src/Utils/miniaudio/miniaudio_utils.h
    src/Utils/pffft/simd/pf_avx_double.h
    src/Utils/pffft/simd/pf_double.h
    src/Utils/pffft/simd/pf_neon_double_from_avx.h
    src/Utils/pffft/simd/pf_neon_double.h
    src/Utils/pffft/simd/pf_scalar_double.h
    src/Utils/pffft/simd/pf_sse2_double.h
    src/Utils/pffft/pffft_double.cpp
    src/Utils/pffft/pffft_double.h
    src/Utils/pffft/pffft_private_impl.h
    src/Utils/intrusive_list.h
    src/Utils/Utils.h
)

# Includes for this project
include_directories(src
    include
    ${AM_FLATBUFFERS_GENERATED_INCLUDES_DIR}
)

add_library(Static STATIC ${SA_SOURCE})
target_compile_definitions(Static PUBLIC AM_BUILDSYSTEM_STATIC)

add_library(Shared SHARED ${SA_SOURCE})
target_compile_definitions(Shared PUBLIC AM_BUILDSYSTEM_SHARED)

add_custom_target(amplitude_build_binary_schemas
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/scripts/build_schemas.py --output ${CMAKE_CURRENT_SOURCE_DIR}/schemas
)

set(AM_BUILDSYSTEM_TYPE "Static" "Shared")

foreach(build_type ${AM_BUILDSYSTEM_TYPE})
    string(TOLOWER ${build_type} build_type_l)

    set_target_properties(${build_type}
        PROPERTIES
            OUTPUT_NAME ${PROJECT_NAME}
            ARCHIVE_OUTPUT_DIRECTORY ${build_type_l}
    )

    target_compile_definitions(${build_type} PRIVATE AM_BUILDSYSTEM_BUILDING_AMPLITUDE)

    target_link_libraries(${build_type}
        PRIVATE
            flatbuffers::flatbuffers SampleRate::samplerate xsimd
        PUBLIC
            mimalloc-static
    )

    add_dependencies(${build_type}
        generated_includes
        amplitude_build_binary_schemas
    )

    if(UNIX)
        target_link_libraries(${build_type} PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
    endif()

    if(APPLE)
        target_link_libraries(${build_type} PUBLIC ${ACCELERATE_FRAMEWORK})
        target_compile_definitions(${build_type} PUBLIC AM_FFT_APPLE_ACCELERATE)
    endif()
endforeach()

add_subdirectory(tools/amac)

if (BUILD_SAMPLES OR UNIT_TESTS)
    add_custom_target(ss_amplitude_audio_sample_project
        COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/scripts/build_assets.py --output ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/sample_project
    )

    add_dependencies(ss_amplitude_audio_sample_project
        amplitude_build_binary_schemas
    )
endif()

if(BUILD_SAMPLES)
    add_subdirectory(samples)
endif()

if(UNIT_TESTS)
    add_subdirectory(tests)
endif()

# #####################################################
# INSTALL

install(
    TARGETS Static
    EXPORT AmplitudeAudioSDKConfig
    ARCHIVE DESTINATION ${AM_LIB_DESTINATION}/static
)

install(
    TARGETS Shared
    EXPORT AmplitudeAudioSDKConfig
    ARCHIVE DESTINATION ${AM_LIB_DESTINATION}/shared
    LIBRARY DESTINATION ${AM_LIB_DESTINATION}/shared
    RUNTIME DESTINATION ${AM_LIB_DESTINATION}/shared
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION ${AM_INC_DESTINATION}
    FILES_MATCHING PATTERN "*.h*"
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/schemas/
    DESTINATION ${AM_SHM_DESTINATION}
    FILES_MATCHING PATTERN "*.bfbs"
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cmake/
    DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake"
    FILES_MATCHING PATTERN "*.cmake"
)

install(
    EXPORT AmplitudeAudioSDKConfig
    DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake"
    NAMESPACE SparkyStudios::Audio::Amplitude::SDK::
)

export(
    TARGETS Static Shared
    NAMESPACE SparkyStudios::Audio::Amplitude::SDK::
    FILE "${CMAKE_CURRENT_BINARY_DIR}/AmplitudeAudioSDKConfig.cmake"
)

export(PACKAGE ${PROJECT_NAME})
