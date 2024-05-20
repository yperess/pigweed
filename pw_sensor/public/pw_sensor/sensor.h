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
#pragma once

#include <limits>

#include "pw_sensor/config.h"
#include "pw_sensor/generated/sensor_constants.h"
#include "pw_sensor/types.h"

namespace pw::sensor {

template <size_t kAttributeCount = std::numeric_limits<size_t>::max()>
class Sensor : public Configurable {
 public:
  ConfigurationFuture GetConfiguration(SensorContextBase& cx,
                                       ConfigurationBase& out) override {
    return ConfigurationFuture(
        cx, std::move(GetConfigurationGetWork()), attributes_, out);
  }

 protected:
  constexpr Sensor(const std::array<Attribute, kAttributeCount>& attributes)
      : attributes_(attributes) {}

  virtual ConfigurationFuture::PendFn GetConfigurationGetWork() = 0;

 private:
  Configuration<kAttributeCount> attributes_;
};

}  // namespace pw::sensor
