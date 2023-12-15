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

#include <Core/EntityInternalState.h>

using namespace SparkyStudios::Audio::Amplitude;

TEST_CASE("Entity Tests", "[entity][core][amplitude]")
{
    EntityInternalState state;
    state.SetId(1);

    fplutil::intrusive_list entity_list(&EntityInternalState::node);
    entity_list.push_back(state);

    SECTION("can be used without a wrapper")
    {
        WHEN("the ID changes")
        {
            state.SetId(5);

            THEN("it returns the new ID")
            {
                REQUIRE(state.GetId() == 5);
            }
        }

        WHEN("the location changes")
        {
            const auto lastLocation = state.GetLocation();
            const auto location = AM_V3(10, 20, 30);
            state.SetLocation(location);

            THEN("it returns the new location")
            {
                REQUIRE(AM_EqV3(state.GetLocation(), location));
            }

            AND_WHEN("an update occurs")
            {
                state.Update();

                THEN("It updates the velocity")
                {
                    const auto& velocity = location - lastLocation;

                    REQUIRE(AM_EqV3(state.GetVelocity(), velocity));
                }
            }
        }

        WHEN("the orientation changes")
        {
            const auto direction = AM_V3(1, 0, 0);
            const auto up = AM_V3(0, 0, 1);
            state.SetOrientation(direction, up);

            THEN("it returns the new orientation")
            {
                REQUIRE(AM_EqV3(state.GetDirection(), direction));
                REQUIRE(AM_EqV3(state.GetUp(), up));
            }
        }

        WHEN("the obstruction changes")
        {
            constexpr AmReal32 obstruction = 0.67f;
            state.SetObstruction(obstruction);

            THEN("it returns the new obstruction")
            {
                REQUIRE(state.GetObstruction() == obstruction);
            }
        }

        WHEN("the occlusion changes")
        {
            constexpr AmReal32 occlusion = 0.43f;
            state.SetOcclusion(occlusion);

            THEN("it returns the new obstruction")
            {
                REQUIRE(state.GetOcclusion() == occlusion);
            }
        }

        WHEN("an environment factor changes")
        {
            constexpr AmEnvironmentID environment = 1;
            constexpr AmReal32 factor = 0.56f;
            state.SetEnvironmentFactor(environment, factor);

            THEN("it returns the new environment factor")
            {
                REQUIRE(state.GetEnvironmentFactor(environment) == factor);
            }
        }

        SECTION("returns 0 as the environment factor for an unregistered environment ID")
        {
            REQUIRE(state.GetEnvironmentFactor(12345) == 0.0f);
        }
    }

    SECTION("can be used with a wrapper")
    {
        Entity wrapper(&state);

        SECTION("can return the correct ID")
        {
            REQUIRE(wrapper.GetId() == 1);
        }

        WHEN("the location changes")
        {
            const auto lastLocation = state.GetLocation();
            const auto location = AM_V3(10, 20, 30);
            wrapper.SetLocation(location);

            THEN("it returns the new location")
            {
                REQUIRE(AM_EqV3(wrapper.GetLocation(), location));
            }

            AND_WHEN("an update occurs")
            {
                wrapper.Update();

                THEN("It updates the velocity")
                {
                    const auto& velocity = location - lastLocation;

                    REQUIRE(AM_EqV3(wrapper.GetVelocity(), velocity));
                }
            }
        }

        WHEN("the orientation changes")
        {
            const auto direction = AM_V3(1, 0, 0);
            const auto up = AM_V3(0, 0, 1);
            wrapper.SetOrientation(direction, up);

            THEN("it returns the new orientation")
            {
                REQUIRE(AM_EqV3(state.GetDirection(), direction));
                REQUIRE(AM_EqV3(state.GetUp(), up));
            }
        }

        WHEN("the obstruction changes")
        {
            constexpr AmReal32 obstruction = 0.67f;
            wrapper.SetObstruction(obstruction);

            THEN("it returns the new obstruction")
            {
                REQUIRE(wrapper.GetObstruction() == obstruction);
            }
        }

        WHEN("the occlusion changes")
        {
            constexpr AmReal32 occlusion = 0.43f;
            wrapper.SetOcclusion(occlusion);

            THEN("it returns the new obstruction")
            {
                REQUIRE(wrapper.GetOcclusion() == occlusion);
            }
        }

        WHEN("the internal state wrapper is cleared")
        {
            wrapper.Clear();

            THEN("it is no longer valid")
            {
                REQUIRE_FALSE(wrapper.Valid());
            }
        }

        WHEN("an environment factor changes")
        {
            constexpr AmEnvironmentID environment = 1;
            constexpr AmReal32 factor = 0.56f;
            wrapper.SetEnvironmentFactor(environment, factor);

            THEN("it returns the new environment factor")
            {
                REQUIRE(wrapper.GetEnvironmentFactor(environment) == factor);
            }
        }

        SECTION("returns 0 as the environment factor for an unregistered environment ID")
        {
            REQUIRE(wrapper.GetEnvironmentFactor(12345) == 0.0f);
        }
    }

    SECTION("cannot create a valid wrapper with a null state")
    {
        Entity wrapper2(nullptr);
        REQUIRE_FALSE(wrapper2.Valid());

        Entity wrapper3;
        REQUIRE_FALSE(wrapper3.Valid());
    }
}