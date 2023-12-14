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

set(PROJECT_NAME ss_amplitude_audio_tests)

project(${PROJECT_NAME})

find_package(Catch2 CONFIG REQUIRED)
add_executable(${PROJECT_NAME} version.cpp filesystem.cpp main.cpp
        thread.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE Catch2::Catch2 Static)

add_dependencies(${PROJECT_NAME}
    Static
    ss_amplitude_audio_sample_project
)

include(CTest)
include(Catch)
catch_discover_tests(${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)
