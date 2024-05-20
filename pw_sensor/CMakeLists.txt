# Copyright 2024 The Pigweed Authors
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

include($ENV{PW_ROOT}/pw_build/pigweed.cmake)
#include($ENV{PW_ROOT}/pw_sensor/backend.cmake)
include($ENV{PW_ROOT}/pw_sensor/sensor.cmake)

pw_sensor_library(pw_sensor.constants
  OUT_HEADER
    pw_sensor/generated/sensor_constants.h
  DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/py/pw_sensor/sensor_desc.py
    ${CMAKE_CURRENT_LIST_DIR}/py/pw_sensor/constants_generator.py
    ${CMAKE_CURRENT_LIST_DIR}/attributes.yaml
    ${CMAKE_CURRENT_LIST_DIR}/channels.yaml
    ${CMAKE_CURRENT_LIST_DIR}/triggers.yaml
  INCLUDES
    $ENV{PW_ROOT}
  SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/sensor.yaml
)

pw_add_library(pw_sensor STATIC
  SOURCES
    configuration_future.cc
  HEADERS
    public/pw_sensor/internal/future.h
    public/pw_sensor/config.h
    public/pw_sensor/context.h
    public/pw_sensor/sensor.h
    public/pw_sensor/types.h
  PUBLIC_INCLUDES
    public
  PUBLIC_DEPS
    pw_async2.dispatcher
    pw_async2.poll
    pw_containers
    pw_function
    pw_sensor.constants
    pw_tokenizer
)

pw_add_test(pw_sensor.constants_test
  SOURCES
    attributes_test.cc
    channels_test.cc
    triggers_test.cc
    units_test.cc
  PRIVATE_DEPS
    pw_sensor
  GROUPS
    modules
    pw_sensor
)

pw_add_test(pw_sensor.configuration_test
  SOURCES
    configuration_test.cc
    sensor_configuration_test.cc
  PRIVATE_DEPS
    pw_sensor
  GROUPS
    modules
    pw_sensor
)
