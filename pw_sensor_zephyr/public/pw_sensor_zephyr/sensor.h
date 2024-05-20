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

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#include "pw_async2/poll.h"
#include "pw_sensor/generated/sensor_constants.h"
#include "pw_sensor/sensor.h"
#include "pw_sensor/types.h"

namespace pw::sensor::zephyr {

template <size_t kAttributeCount = std::numeric_limits<size_t>::max()>
class ZephyrSensor : public Sensor<kAttributeCount> {
 public:
  constexpr ZephyrSensor(
      const struct device* dev,
      const std::array<Attribute, kAttributeCount>& attributes)
      : Sensor<kAttributeCount>(attributes), dev_(dev) {}
  using Sensor<kAttributeCount>::GetConfiguration;

 protected:
  const struct device* dev_;
};

PW_SENSOR_ATTRIBUTE_INSTANCE(kMagneticFieldSampleRate,
                             channels::kMagneticField,
                             attributes::kSampleRate,
                             units::kFrequency,
                             uint64_t);

class AKM09918C : public ZephyrSensor<1> {
 public:
  AKM09918C(const struct device* dev)
      : ZephyrSensor<1>(dev, {Attribute::Build<kMagneticFieldSampleRate>()}) {}

 protected:
  ConfigurationFuture::PendFn GetConfigurationGetWork() override {
    return [this](SensorContextBase& cx,
                  ConfigurationBase& in,
                  ConfigurationBase& out) -> pw::async2::Poll<> {
      struct sensor_value val;
      int rc = sensor_attr_get(this->dev_,
                               SENSOR_CHAN_MAGN_XYZ,
                               SENSOR_ATTR_SAMPLING_FREQUENCY,
                               &val);
      if (rc != 0) {
        return async2::Ready();
      }
      out.SetAttribute<kMagneticFieldSampleRate>(val.val1);
      return async2::Ready();
    };
  }
};

}  // namespace pw::sensor::zephyr
