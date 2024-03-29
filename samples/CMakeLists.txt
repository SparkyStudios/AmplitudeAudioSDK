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

set(PROJECT_NAME ss_amplitude_audio_samples)

project(${PROJECT_NAME})

add_custom_target(ss_amplitude_audio_sample_assets
    COMMAND
        python3 ${CMAKE_CURRENT_SOURCE_DIR}/../scripts/build_project.py -p ${CMAKE_SOURCE_DIR}/samples/rawassets -b ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/samples/assets -f ${CMAKE_SOURCE_DIR}/bin/flatc
    WORKING_DIRECTORY
        ${CMAKE_SOURCE_DIR}
)

add_dependencies(ss_amplitude_audio_sample_assets
    amplitude_build_binary_schemas
)

add_subdirectory(sample_01)
add_subdirectory(sample_02)

install(
    CODE "execute_process(COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/../scripts/build_project.py -p ${CMAKE_SOURCE_DIR}/samples/rawassets -b ${CMAKE_INSTALL_PREFIX}/${AM_BIN_DESTINATION}/samples/assets -f ${CMAKE_SOURCE_DIR}/bin/flatc WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})"
)

install(
    TARGETS ss_amplitude_audio_sample_01 ss_amplitude_audio_sample_02
    RUNTIME DESTINATION ${AM_BIN_DESTINATION}/samples
)
