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

cmake_minimum_required(VERSION 3.10)

set(PROJECT_NAME ss_amplitude_audio_sample_01)

project(${PROJECT_NAME})

if(WIN32)
    set(RUNTIME_OUTPUT_DIRECTORY "win")
elseif(APPLE)
    set(RUNTIME_OUTPUT_DIRECTORY "osx")
    set(CMAKE_MACOSX_RPATH 1)
    list(APPEND CMAKE_INSTALL_RPATH "@executable_path" "@executable_path/../../../lib/${RUNTIME_OUTPUT_DIRECTORY}/shared/")
else()
    set(RUNTIME_OUTPUT_DIRECTORY "linux")
    list(APPEND CMAKE_INSTALL_RPATH "${ORIGIN}" "${ORIGIN}/../../../lib/${RUNTIME_OUTPUT_DIRECTORY}/shared/")
endif()

set(ss_amplitude_audio_sample_01_SRCS
    main.cpp
)

add_executable(ss_amplitude_audio_sample_01 ${ss_amplitude_audio_sample_01_SRCS})

set_target_properties(ss_amplitude_audio_sample_01
    PROPERTIES
        OUTPUT_NAME sample_01
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/samples
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC Shared
)

add_dependencies(${PROJECT_NAME}
    Shared
    ss_amplitude_audio_sample_project
)
