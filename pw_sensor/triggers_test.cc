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

constexpr uint32_t kExpectedDataReadyTriggerType =
    PW_TOKENIZE_STRING_DOMAIN("PW_SENSOR_TRIGGER_TYPE", "data ready");

static_assert(triggers::kDataReady::kTriggerType ==
              kExpectedDataReadyTriggerType);

}  // namespace
}  // namespace pw::sensor
