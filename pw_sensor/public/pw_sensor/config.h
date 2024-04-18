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
#include <type_traits>

#include "pw_containers/vector.h"
#include "pw_result/result.h"
#include "pw_sensor/types.h"
#include "pw_status/status.h"

namespace pw::sensor {

class Attribute {
 public:
  using _float_type = double;
  using _signed_type = int64_t;
  using _unsigned_type = uint64_t;

  template <typename value_type>
  std::enable_if_t<std::is_integral_v<value_type>, pw::Result<value_type>>
  GetValue(const value_type& = value_type()) const {
    switch (_value_type) {
      case InternalType::UNASSIGNED:
        return pw::Status::Unknown();
      case InternalType::FLOAT:
        // We're storing a float internally, there's no good way to guarantee
        // that we won't lose resolution
        return pw::Status::InvalidArgument();
      case InternalType::UNSIGNED_INT: {
        // No need to check the lower bound because internal representation is
        // unsigned and will never be smaller than 0.
        if (_value._unsigned >
            (_unsigned_type)std::numeric_limits<value_type>::max()) {
          // Internal value is out of bounds for the requested type
          return pw::Status::InvalidArgument();
        }
        return static_cast<value_type>(_value._unsigned);
      }
      case InternalType::SIGNED_INT: {
        if (_value._signed <
                (_signed_type)std::numeric_limits<value_type>::min() ||
            _value._signed >
                (_signed_type)std::numeric_limits<value_type>::max()) {
          // Internal value is out of bounds for the requested type
          return pw::Status::InvalidArgument();
        }
        return static_cast<value_type>(_value._signed);
      }
    }
  }

  template <typename value_type>
  std::enable_if_t<std::is_floating_point_v<value_type>, pw::Result<value_type>>
  GetValue(const value_type& = value_type()) const {
    switch (_value_type) {
      case InternalType::UNASSIGNED:
        return pw::Status::Unknown();
      case InternalType::SIGNED_INT: {
        if (!IsFloatInRange<_signed_type, value_type>(_value._signed)) {
          // Internal value is out of bounds for the requested type
          return pw::Status::InvalidArgument();
        }
        return static_cast<value_type>(_value._signed);
      }
      case InternalType::UNSIGNED_INT: {
        if (!IsFloatInRange<_signed_type, value_type>(_value._unsigned)) {
          // Internal value is out of bounds for the requested type
          return pw::Status::InvalidArgument();
        }
        return static_cast<value_type>(_value._unsigned);
      }
      case InternalType::FLOAT: {
        if (!IsFloatInRange<_float_type, value_type>(_value._float)) {
          // Internal value is out of bounds for the requested type
          return pw::Status::InvalidArgument();
        }
        return static_cast<value_type>(_value._float);
      }
    }
  }

  template <typename value_type>
  std::enable_if_t<std::is_integral_v<value_type> &&
                       std::is_unsigned_v<value_type>,
                   pw::Status>
  SetValue(value_type value) {
    switch (_value_type) {
      case InternalType::UNASSIGNED:
        return pw::Status::Unknown();
      case InternalType::SIGNED_INT: {
        if (value > std::numeric_limits<_signed_type>::max()) {
          return pw::Status::InvalidArgument();
        }
        _value._signed = static_cast<_signed_type>(value);
        return pw::OkStatus();
      }
      case InternalType::FLOAT: {
        if (value > std::numeric_limits<_float_type>::max()) {
          return pw::Status::InvalidArgument();
        }
        _value._float = static_cast<_float_type>(value);
        return pw::OkStatus();
      }
      case InternalType::UNSIGNED_INT: {
        _value._unsigned = static_cast<_unsigned_type>(value);
        return pw::OkStatus();
      }
    }
  }

  template <typename value_type>
  std::enable_if_t<std::is_integral_v<value_type> &&
                       std::is_signed_v<value_type>,
                   pw::Status>
  SetValue(value_type value) {
    switch (_value_type) {
      case InternalType::UNASSIGNED:
        return pw::Status::Unknown();
      case InternalType::FLOAT: {
        if (!IsFloatInRange<value_type, _float_type>(value)) {
          return pw::Status::InvalidArgument();
        }
        _value._float = static_cast<_float_type>(value);
        return pw::OkStatus();
      }
      case InternalType::SIGNED_INT: {
        _value._signed = static_cast<_signed_type>(value);
        return pw::OkStatus();
      }
      case InternalType::UNSIGNED_INT: {
        if (value < 0) {
          return pw::Status::InvalidArgument();
        }
        _value._unsigned = static_cast<_unsigned_type>(value);
        return pw::OkStatus();
      }
    }
  }

  template <typename value_type>
  std::enable_if_t<std::is_floating_point_v<value_type>, pw::Status> SetValue(
      value_type value) {
    switch (_value_type) {
      case InternalType::UNASSIGNED:
        return pw::Status::Unknown();
      case InternalType::SIGNED_INT:
      case InternalType::UNSIGNED_INT:
        // Can't use an int type internally to store a float
        return pw::Status::InvalidArgument();
      case InternalType::FLOAT: {
        _value._float = static_cast<_float_type>(value);
        return pw::OkStatus();
      }
    }
  }

  template <typename AttributeInstance>
  bool IsInstance() const {
    return _measurement_type == AttributeInstance::kMeasurementType &&
           _attribute_type == AttributeInstance::kAttributeType;
  }

  bool EquivalentTo(const Attribute& other) const {
    return _measurement_type == other._measurement_type &&
           _attribute_type == other._attribute_type;
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
      : _measurement_type(measurement_type),
        _attribute_type(attribute_type),
        _value_type(value_type) {}

  const uint64_t _measurement_type;
  const uint32_t _attribute_type;
  const InternalType _value_type = InternalType::UNASSIGNED;
  union {
    _float_type _float;
    _unsigned_type _unsigned;
    _signed_type _signed;
  } _value = {0};
};

template <size_t kAttributeCount = size_t(-1)>
class Configuration {
 public:
  static_assert(kAttributeCount > 0);
  constexpr Configuration(
      const std::array<Attribute, kAttributeCount>& attributes)
      : _attributes(attributes.begin(), attributes.end()) {}
  constexpr Configuration() = default;

  template <typename AttributeInstance,
            typename ValueType,
            typename = std::enable_if_t<
                std::is_convertible_v<ValueType,
                                      typename AttributeInstance::value_type>>>
  pw::Status SetAttribute(ValueType value) {
    for (auto& it : _attributes) {
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
    for (auto& it : _attributes) {
      if (!it.template IsInstance<AttributeInstance>()) {
        continue;
      }
      return it.template GetValue<ValueType>();
    }
    return pw::Status::NotFound();
  }

  template <typename AttributeInstance>
  pw::Status AddAttribute() {
    return AddAttribute(Attribute::Build<AttributeInstance>());
  }

  pw::Status AddAttribute(const Attribute& attribute) {
    for (auto& it : _attributes) {
      if (it.EquivalentTo(attribute)) {
        // We already have this attribute
        return pw::Status::AlreadyExists();
      }
    }
    if (_attributes.full()) {
      return pw::Status::ResourceExhausted();
    }
    _attributes.push_back(attribute);
    return pw::OkStatus();
  }

 private:
  pw::Vector<Attribute, kAttributeCount> _attributes;
};

class ConfigurationFuture {};

class Configurable {
 public:
  virtual ~Configurable() {}
  virtual ConfigurationFuture GetConfiguration(Configuration<>& out) = 0;
};

}  // namespace pw::sensor
