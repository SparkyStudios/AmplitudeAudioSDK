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

#include <catch2/catch_test_macros.hpp>

#include <SparkyStudios/Audio/Amplitude/Amplitude.h>

using namespace SparkyStudios::Audio::Amplitude;

TEST_CASE("Version Tests", "[version][amplitude]")
{
    const auto& version = amVersion;

    SECTION("have the correct major version number")
    {
        REQUIRE(version.major == AM_VERSION_MAJOR);
    }

    SECTION("have the correct minor version number")
    {
        REQUIRE(version.minor == AM_VERSION_MINOR);
    }

    SECTION("have the correct patch version number")
    {
        REQUIRE(version.revision == AM_VERSION_PATCH);
    }

    SECTION("have the correct version string")
    {
        REQUIRE(version.text == "Amplitude Audio SDK " AM_TO_STRING(AM_VERSION_MAJOR) "." AM_TO_STRING(AM_VERSION_MINOR) "." AM_TO_STRING(AM_VERSION_PATCH));
    }
}