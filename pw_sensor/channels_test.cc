// Copyright 2024 The Pigweed Authors
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy of
// the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.

#include "pw_sensor/sensor.h"
#include "pw_tokenizer/tokenize.h"
#include "pw_unit_test/framework.h"

namespace pw::sensor {

namespace {

#define TEST_CHANNEL(_name, _expected_name_str, _units, type)            \
  constexpr auto kExpected##_name##MeasurementName =                     \
      PW_TOKENIZE_STRING_DOMAIN("PW_SENSOR_MEASUREMENT_TYPE",            \
                                _expected_name_str);                     \
  constexpr auto kExpected##_name##MeasurementType =                     \
      (static_cast<uint64_t>(kExpected##_name##MeasurementName) << 32) | \
      _units::kUnitType;                                                 \
  static_assert(channels::k##_name::kMeasurementName ==                  \
                kExpected##_name##MeasurementName);                      \
  static_assert(channels::k##_name::kUnitType == _units::kUnitType);     \
  static_assert(channels::k##_name::kMeasurementType ==                  \
                kExpected##_name##MeasurementType)

TEST_CHANNEL(AmbientTemperature,
             "ambient temperature",
             units::kTemperature,
             double);

TEST_CHANNEL(Acceleration, "acceleration", units::kAcceleration, double);

TEST_CHANNEL(DieTemperature, "die temperature", units::kTemperature, double);

TEST_CHANNEL(MagneticField, "magnetic field", units::kMagneticField, double);

TEST_CHANNEL(RotationalVelocity,
             "rotational velocity",
             units::kRotationalVelocity,
             double);

}  // namespace

}  // namespace pw::sensor
