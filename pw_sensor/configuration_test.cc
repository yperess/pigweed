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
#include <limits>

#include "pw_sensor/config.h"
#include "pw_sensor/sensor.h"
#include "pw_sensor/types.h"
#include "pw_status/status.h"
#include "pw_tokenizer/tokenize.h"
#include "pw_unit_test/framework.h"

namespace pw::sensor {
namespace {

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

TEST(Attribute, SetFloatAttributeValue_UsingInt) {
  auto attribute = Attribute::Build<kFloatAttributeInstance>();
  // Set value using signed int
  EXPECT_OK(attribute.SetValue(15));

  // Get the same value using a float
  Result<float> f_result = attribute.GetValue<float>();
  EXPECT_OK(f_result.status());
  EXPECT_FLOAT_EQ(15.0f, *f_result);

  // Get the same value using a double
  Result<double> d_result = attribute.GetValue<double>();
  EXPECT_OK(d_result.status());
  EXPECT_FLOAT_EQ(15.0, *d_result);

  // Get the same value using a signed int (should fail)
  Result<int> i_result = attribute.GetValue<int>();
  EXPECT_EQ(Status::InvalidArgument(), i_result.status());

  // Get the same value using an unsigned int (should fail)
  Result<unsigned int> u_result = attribute.GetValue<unsigned int>();
  EXPECT_EQ(Status::InvalidArgument(), u_result.status());
}

TEST(Attribute, SetFloatAttributeValue_Limits) {
  auto attribute = Attribute::Build<kFloatAttributeInstance>();

  // Set value to uint64_t::max
  EXPECT_OK(attribute.SetValue(std::numeric_limits<uint64_t>::max()));
  auto uint64_max_result = attribute.GetValue<double>();
  EXPECT_OK(uint64_max_result.status());
  EXPECT_DOUBLE_EQ(static_cast<double>(std::numeric_limits<uint64_t>::max()),
                   *uint64_max_result);

  // Set value to int64_t::max
  EXPECT_OK(attribute.SetValue(std::numeric_limits<int64_t>::max()));
  auto int64_max_result = attribute.GetValue<double>();
  EXPECT_OK(int64_max_result.status());
  EXPECT_DOUBLE_EQ(static_cast<double>(std::numeric_limits<int64_t>::max()),
                   *int64_max_result);

  // Set value to int64_t::min
  EXPECT_OK(attribute.SetValue(std::numeric_limits<int64_t>::min()));
  auto int64_min_result = attribute.GetValue<double>();
  EXPECT_OK(int64_min_result.status());
  EXPECT_DOUBLE_EQ(static_cast<double>(std::numeric_limits<int64_t>::min()),
                   *int64_min_result);
}

TEST(Attribute, SetFloatAttributeValue_UsingFloat) {
  auto attribute = Attribute::Build<kFloatAttributeInstance>();
  // Set value using float
  EXPECT_OK(attribute.SetValue(15.0f));

  // Get the same value using a float
  Result<float> f_result = attribute.GetValue<float>();
  EXPECT_OK(f_result.status());
  EXPECT_FLOAT_EQ(15.0f, *f_result);

  // Get the same value using a double
  Result<double> d_result = attribute.GetValue<double>();
  EXPECT_OK(d_result.status());
  EXPECT_FLOAT_EQ(15.0, *d_result);

  // Get the same value using a signed int
  Result<int> i_result = attribute.GetValue<int>();
  EXPECT_EQ(Status::InvalidArgument(), i_result.status());

  // Get the same value using an unsigned int
  Result<unsigned int> u_result = attribute.GetValue<unsigned int>();
  EXPECT_EQ(Status::InvalidArgument(), u_result.status());
}

TEST(Attribute, SetIntAttributeValue_UsingInt) {
  auto attribute = Attribute::Build<kIntAttributeInstance>();
  // Set value using a signed int
  EXPECT_OK(attribute.SetValue(7));

  // Get the same value using a signed int
  Result<int> i_result = attribute.GetValue<int>();
  EXPECT_OK(i_result.status());
  EXPECT_EQ(7, *i_result);

  // Get the same value using an unsigned int
  Result<unsigned> u_result = attribute.GetValue<unsigned>();
  EXPECT_OK(u_result.status());
  EXPECT_EQ(7u, *u_result);

  // Get the same value using a float
  auto f_result = attribute.GetValue<float>();
  EXPECT_OK(f_result.status());
  EXPECT_FLOAT_EQ(7.0f, *f_result);

  // Get the same value using a double
  auto d_result = attribute.GetValue<double>();
  EXPECT_OK(d_result.status());
  EXPECT_DOUBLE_EQ(7.0, *d_result);
}

TEST(Configuration, CantSetMissingAttribute) {
  Configuration<1> config;
  EXPECT_EQ(pw::Status::NotFound(),
            config.SetAttribute<kIntAttributeInstance>(0));
}

TEST(Configuration, AddAttributeFromObject) {
  Configuration<1> config;
  auto attribute0 = Attribute::Build<kIntAttributeInstance>();
  auto attribute1 = Attribute::Build<kFloatAttributeInstance>();
  EXPECT_OK(config.AddAttribute(attribute0));
  EXPECT_EQ(pw::Status::AlreadyExists(), config.AddAttribute(attribute0));
  EXPECT_EQ(pw::Status::ResourceExhausted(), config.AddAttribute(attribute1));
}

TEST(Configuration, AddAttributeByType) {
  Configuration<1> config;
  EXPECT_OK(config.AddAttribute<kIntAttributeInstance>());
  EXPECT_EQ(pw::Status::AlreadyExists(),
            config.AddAttribute<kIntAttributeInstance>());
  EXPECT_EQ(pw::Status::ResourceExhausted(),
            config.AddAttribute<kFloatAttributeInstance>());
}

TEST(Configuration, InitFromArray) {
  Configuration<1> config({Attribute::Build<kIntAttributeInstance>()});
  EXPECT_EQ(pw::Status::AlreadyExists(),
            config.AddAttribute<kIntAttributeInstance>());
  EXPECT_EQ(pw::Status::ResourceExhausted(),
            config.AddAttribute<kFloatAttributeInstance>());
}

TEST(Configuration, SetIntAttribute) {
  Configuration<1> config({Attribute::Build<kIntAttributeInstance>()});
  EXPECT_OK(config.SetAttribute<kIntAttributeInstance>(27));

  // Get result as int
  auto i_result = config.GetAttribute<kIntAttributeInstance, int>();
  EXPECT_OK(i_result.status());
  EXPECT_EQ(27, *i_result);

  // Get result as unsigned int
  auto u_result = config.GetAttribute<kIntAttributeInstance, unsigned>();
  EXPECT_OK(u_result.status());
  EXPECT_EQ(27u, *u_result);

  // Get result as float
  auto f_result = config.GetAttribute<kIntAttributeInstance, float>();
  EXPECT_OK(f_result.status());
  EXPECT_FLOAT_EQ(27.0f, *f_result);

  // Get result as double
  auto d_result = config.GetAttribute<kIntAttributeInstance, double>();
  EXPECT_OK(d_result.status());
  EXPECT_DOUBLE_EQ(27.0, *d_result);
}

TEST(Configuration, SetFloatAttribute) {
  Configuration<1> config({Attribute::Build<kFloatAttributeInstance>()});
  EXPECT_OK(config.SetAttribute<kFloatAttributeInstance>(-33.5f));

  // Get result as int
  auto i_result = config.GetAttribute<kFloatAttributeInstance, int>();
  EXPECT_EQ(pw::Status::InvalidArgument(), i_result.status());

  // Get result as unsigned int
  auto u_result = config.GetAttribute<kFloatAttributeInstance, unsigned>();
  EXPECT_EQ(pw::Status::InvalidArgument(), u_result.status());

  // Get result as float
  auto f_result = config.GetAttribute<kFloatAttributeInstance, float>();
  EXPECT_OK(f_result.status());
  EXPECT_FLOAT_EQ(-33.5f, *f_result);

  // Get result as double
  auto d_result = config.GetAttribute<kFloatAttributeInstance, double>();
  EXPECT_OK(d_result.status());
  EXPECT_DOUBLE_EQ(-33.5, *d_result);
}

}  // namespace
}  // namespace pw::sensor
