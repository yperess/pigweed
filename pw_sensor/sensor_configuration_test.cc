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

#include <cstdint>
#include <iostream>
#include <limits>
#include <utility>

#include "pw_sensor/config.h"
#include "pw_sensor/context.h"
#include "pw_sensor/sensor.h"
#include "pw_sensor/types.h"
#include "pw_status/status.h"
#include "pw_tokenizer/tokenize.h"
#include "pw_unit_test/framework.h"

namespace pw::sensor {
namespace {

using ::pw::async2::Context;
using ::pw::async2::Dispatcher;
using ::pw::async2::Pending;
using ::pw::async2::Poll;
using ::pw::async2::Ready;
using ::pw::async2::Task;

#define EXPECT_OK(expression) EXPECT_EQ(::pw::OkStatus(), expression)
#define ASSERT_OK(expression) ASSERT_EQ(::pw::OkStatus(), expression)

PW_SENSOR_UNIT_TYPE(kTestUnit, "TEST_UNITS", "test units", "things");

PW_SENSOR_MEASUREMENT_TYPE(
    kFloatMeasurement, "TEST_MEASUREMENT", "sample rate", kTestUnit, double);
PW_SENSOR_MEASUREMENT_TYPE(
    kUintMeasurement, "TEST_MEASUREMENT", "step counter", kTestUnit, uint64_t);

PW_SENSOR_ATTRIBUTE_TYPE(kTestAttribute, "TEST_ATTRIBUTE", "test attribute");

PW_SENSOR_ATTRIBUTE_INSTANCE(kFloatAttributeInstance,
                             kFloatMeasurement,
                             kTestAttribute,
                             kTestUnit,
                             float);

PW_SENSOR_ATTRIBUTE_INSTANCE(
    kIntAttributeInstance, kUintMeasurement, kTestAttribute, kTestUnit, int);

class TestSensor : public Sensor<1> {
 public:
  constexpr TestSensor()
      : Sensor<1>({Attribute::Build<kIntAttributeInstance>()}) {}

  int configuration_get_count = 0;
  int configuration_get_threshold = 1;

 private:
  ConfigurationFuture::PendFn GetConfigurationGetWork() override {
    return [this](SensorContextBase& cx,
                  ConfigurationBase& in,
                  ConfigurationBase& out) -> pw::async2::Poll<> {
      (void)cx;
      (void)in;
      (void)out;
      std::cout << "configuration_get_count=" << this->configuration_get_count
                << ", configuration_get_threshold="
                << this->configuration_get_threshold << std::endl;
      if (this->configuration_get_count++ >=
          this->configuration_get_threshold) {
        return Ready();
      }
      return Pending();
    };
  }
};

class TestTask : public Task {
 public:
  TestTask(ConfigurationFuture&& future)
      : future_(std::move(future)), last_result_(Pending()) {}
  ConfigurationFuture future_;
  Poll<> last_result_;

 private:
  Poll<> DoPend(Context& cx) override {
    last_result_ = future_.Pend(cx);
    return last_result_;
  }
};

class TestContext : public SensorContextBase {};

TEST(SensorConfiguration, GetConfiguration) {
  TestSensor sensor;
  TestContext sensor_context;
  Configuration<1> output;
  TestTask task(sensor.GetConfiguration(sensor_context, output));

  Dispatcher dispatcher;
  dispatcher.Post(task);
  dispatcher.RunToCompletion();

  ASSERT_TRUE(task.last_result_.IsReady());
  ASSERT_EQ(sensor.configuration_get_count, 2);
}

}  // namespace
}  // namespace pw::sensor
