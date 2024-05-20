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

#include <cmath>
#include <cstddef>
#include <cstring>
#include <limits>
#include <optional>
#include <type_traits>

#include "pw_async2/dispatcher.h"
#include "pw_async2/poll.h"
#include "pw_containers/vector.h"
#include "pw_function/function.h"
#include "pw_result/result.h"
#include "pw_sensor/context.h"
#include "pw_sensor/internal/future.h"
#include "pw_sensor/types.h"
#include "pw_status/status.h"

namespace pw::sensor {

class Attribute {
 public:
  using float_type_ = double;
  using signed_type_ = int64_t;
  using unsigned_type_ = uint64_t;

  template <typename value_type>
  std::enable_if_t<std::is_integral_v<value_type>, pw::Result<value_type>>
  GetValue(const value_type& = value_type()) const {
    switch (value_type_) {
      case InternalType::UNASSIGNED:
        return pw::Status::Unknown();
      case InternalType::FLOAT:
        // We're storing a float internally, there's no good way to guarantee
        // that we won't lose resolution
        return pw::Status::InvalidArgument();
      case InternalType::UNSIGNED_INT: {
        // No need to check the lower bound because internal representation is
        // unsigned and will never be smaller than 0.
        if (value_.unsigned_ >
            (unsigned_type_)std::numeric_limits<value_type>::max()) {
          // Internal value is out of bounds for the requested type
          return pw::Status::InvalidArgument();
        }
        return static_cast<value_type>(value_.unsigned_);
      }
      case InternalType::SIGNED_INT: {
        if (value_.signed_ <
                (signed_type_)std::numeric_limits<value_type>::min() ||
            value_.signed_ >
                (signed_type_)std::numeric_limits<value_type>::max()) {
          // Internal value is out of bounds for the requested type
          return pw::Status::InvalidArgument();
        }
        return static_cast<value_type>(value_.signed_);
      }
    }
    PW_UNREACHABLE;
  }

  template <typename value_type>
  std::enable_if_t<std::is_floating_point_v<value_type>, pw::Result<value_type>>
  GetValue(const value_type& = value_type()) const {
    switch (value_type_) {
      case InternalType::UNASSIGNED:
        return pw::Status::Unknown();
      case InternalType::SIGNED_INT: {
        if (!IsFloatInRange<signed_type_, value_type>(value_.signed_)) {
          // Internal value is out of bounds for the requested type
          return pw::Status::InvalidArgument();
        }
        return static_cast<value_type>(value_.signed_);
      }
      case InternalType::UNSIGNED_INT: {
        if (!IsFloatInRange<signed_type_, value_type>(value_.unsigned_)) {
          // Internal value is out of bounds for the requested type
          return pw::Status::InvalidArgument();
        }
        return static_cast<value_type>(value_.unsigned_);
      }
      case InternalType::FLOAT: {
        if (!IsFloatInRange<float_type_, value_type>(value_.float_)) {
          // Internal value is out of bounds for the requested type
          return pw::Status::InvalidArgument();
        }
        return static_cast<value_type>(value_.float_);
      }
    }
  }

  template <typename value_type>
  std::enable_if_t<std::is_integral_v<value_type> &&
                       std::is_unsigned_v<value_type>,
                   pw::Status>
  SetValue(value_type value) {
    switch (value_type_) {
      case InternalType::UNASSIGNED:
        return pw::Status::Unknown();
      case InternalType::SIGNED_INT: {
        if (value > std::numeric_limits<signed_type_>::max()) {
          return pw::Status::InvalidArgument();
        }
        value_.signed_ = static_cast<signed_type_>(value);
        return pw::OkStatus();
      }
      case InternalType::FLOAT: {
        if (value > std::numeric_limits<float_type_>::max()) {
          return pw::Status::InvalidArgument();
        }
        value_.float_ = static_cast<float_type_>(value);
        return pw::OkStatus();
      }
      case InternalType::UNSIGNED_INT: {
        value_.unsigned_ = static_cast<unsigned_type_>(value);
        return pw::OkStatus();
      }
    }
    PW_UNREACHABLE;
  }

  template <typename value_type>
  std::enable_if_t<std::is_integral_v<value_type> &&
                       std::is_signed_v<value_type>,
                   pw::Status>
  SetValue(value_type value) {
    switch (value_type_) {
      case InternalType::UNASSIGNED:
        return pw::Status::Unknown();
      case InternalType::FLOAT: {
        if (!IsFloatInRange<value_type, float_type_>(value)) {
          return pw::Status::InvalidArgument();
        }
        value_.float_ = static_cast<float_type_>(value);
        return pw::OkStatus();
      }
      case InternalType::SIGNED_INT: {
        value_.signed_ = static_cast<signed_type_>(value);
        return pw::OkStatus();
      }
      case InternalType::UNSIGNED_INT: {
        if (value < 0) {
          return pw::Status::InvalidArgument();
        }
        value_.unsigned_ = static_cast<unsigned_type_>(value);
        return pw::OkStatus();
      }
    }
    PW_UNREACHABLE;
  }

  template <typename value_type>
  std::enable_if_t<std::is_floating_point_v<value_type>, pw::Status> SetValue(
      value_type value) {
    switch (value_type_) {
      case InternalType::UNASSIGNED:
        return pw::Status::Unknown();
      case InternalType::SIGNED_INT:
      case InternalType::UNSIGNED_INT:
        // Can't use an int type internally to store a float
        return pw::Status::InvalidArgument();
      case InternalType::FLOAT: {
        value_.float_ = static_cast<float_type_>(value);
        return pw::OkStatus();
      }
    }
  }

  template <typename AttributeInstance>
  bool IsInstance() const {
    return measurement_type_ == AttributeInstance::kMeasurementType &&
           attribute_type_ == AttributeInstance::kAttributeType;
  }

  bool EquivalentTo(const Attribute& other) const {
    return measurement_type_ == other.measurement_type_ &&
           attribute_type_ == other.attribute_type_;
  }

  template <typename AttributeInstance>
  static constexpr Attribute Build() {
    constexpr Attribute::InternalType value_type =
        std::is_floating_point_v<typename AttributeInstance::value_type>
            ? Attribute::InternalType::FLOAT
        : (std::is_integral_v<typename AttributeInstance::value_type> &&
           std::is_signed_v<typename AttributeInstance::value_type>)
            ? Attribute::InternalType::SIGNED_INT
        : (std::is_integral_v<typename AttributeInstance::value_type> &&
           std::is_unsigned_v<typename AttributeInstance::value_type>)
            ? Attribute::InternalType::UNSIGNED_INT
            : Attribute::InternalType::UNASSIGNED;
    static_assert(value_type != Attribute::InternalType::UNASSIGNED,
                  "Unsupported type");
    return Attribute(AttributeInstance::kMeasurementType,
                     AttributeInstance::kAttributeType,
                     value_type);
  }

 private:
  enum InternalType {
    UNASSIGNED,
    SIGNED_INT,
    UNSIGNED_INT,
    FLOAT,
  };

  template <
      typename value_type,
      typename limit_value_type,
      typename = std::enable_if_t<std::is_floating_point_v<limit_value_type>>>
  static bool IsFloatInRange(value_type value) {
    auto limit =
        std::nextafter(std::numeric_limits<limit_value_type>::max(), -INFINITY);
    return value >= -limit && value <= limit;
  }

  constexpr Attribute(uint64_t measurement_type,
                      uint32_t attribute_type,
                      InternalType value_type)
      : measurement_type_(measurement_type),
        attribute_type_(attribute_type),
        value_type_(value_type) {}

  const uint64_t measurement_type_;
  const uint32_t attribute_type_;
  const InternalType value_type_ = InternalType::UNASSIGNED;
  union {
    float_type_ float_;
    unsigned_type_ unsigned_;
    signed_type_ signed_;
  } value_ = {0};
};

class ConfigurationBase {
 public:
  ConfigurationBase(pw::Vector<Attribute>& attributes)
      : attributes_(attributes) {}

  template <typename AttributeInstance,
            typename ValueType,
            typename = std::enable_if_t<
                std::is_convertible_v<ValueType,
                                      typename AttributeInstance::value_type>>>
  pw::Status SetAttribute(ValueType value) {
    for (auto& it : attributes_) {
      if (!it.template IsInstance<AttributeInstance>()) {
        continue;
      }
      return it.SetValue(value);
    }
    return pw::Status::NotFound();
  }

  template <typename AttributeInstance, typename ValueType>
  std::enable_if_t<
      std::is_convertible_v<ValueType, typename AttributeInstance::value_type>,
      pw::Result<ValueType>>
  GetAttribute(const ValueType& = ValueType()) const {
    for (auto& it : attributes_) {
      if (!it.template IsInstance<AttributeInstance>()) {
        continue;
      }
      return it.template GetValue<ValueType>();
    }
    return pw::Status::NotFound();
  }

  pw::Status AddAttribute(const Attribute& attribute) {
    for (auto& it : attributes_) {
      if (it.EquivalentTo(attribute)) {
        // We already have this attribute
        return pw::Status::AlreadyExists();
      }
    }
    if (attributes_.full()) {
      return pw::Status::ResourceExhausted();
    }
    attributes_.push_back(attribute);
    return pw::OkStatus();
  }

 private:
  pw::Vector<Attribute>& attributes_;
};

template <size_t kAttributeCount = size_t(-1)>
class Configuration : public ConfigurationBase {
 public:
  static_assert(kAttributeCount > 0);
  constexpr Configuration(
      const std::array<Attribute, kAttributeCount>& attributes)
      : Configuration(attributes.cbegin(), attributes.cend()) {
        auto it = attributes.cbegin();
        (void)it;
      }
  constexpr Configuration(
      typename std::array<Attribute, kAttributeCount>::const_iterator begin,
      typename std::array<Attribute, kAttributeCount>::const_iterator end)
      : ConfigurationBase(attributes_), attributes_(begin, end) {}

  constexpr Configuration() : ConfigurationBase(attributes_) {}

  template <typename AttributeInstance, typename ValueType>
  pw::Status SetAttribute(ValueType value) {
    return ConfigurationBase::SetAttribute<AttributeInstance, ValueType>(value);
  }

  template <typename AttributeInstance, typename ValueType>
  pw::Result<ValueType> GetAttribute(const ValueType& = ValueType()) const {
    return ConfigurationBase::GetAttribute<AttributeInstance, ValueType>();
  }

  template <typename AttributeInstance>
  pw::Status AddAttribute() {
    return AddAttribute(Attribute::Build<AttributeInstance>());
  }

  using ConfigurationBase::AddAttribute;

 private:
  pw::Vector<Attribute, kAttributeCount> attributes_;
};

class SensorContextBase;

class ConfigurationFuture {
 public:
 friend class SensorContextBase;
  using PendFn = pw::Function<pw::async2::Poll<>(
      SensorContextBase&, ConfigurationBase&, ConfigurationBase&)>;
  ConfigurationFuture(SensorContextBase& cx,
                      PendFn&& work,
                      ConfigurationBase& in,
                      ConfigurationBase& out)
      : cx_(cx),
        pend_impl_(std::move(work)),
        in_(in),
        out_(out),
        last_result_(pw::async2::Pending()) {
          cx.AddFuture(future_);
        }
  pw::async2::Poll<> Pend(pw::async2::Context& cx);

 private:
  SensorContextBase& cx_;
  PendFn pend_impl_;
  ConfigurationBase& in_;
  ConfigurationBase& out_;

  pw::async2::Poll<> last_result_;
  internal::Future future_;
};

class Configurable {
 public:
  virtual ~Configurable() {}
  virtual ConfigurationFuture GetConfiguration(SensorContextBase& cx,
                                               ConfigurationBase& out) = 0;
};

}  // namespace pw::sensor
