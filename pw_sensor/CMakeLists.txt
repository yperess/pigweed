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

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/include/pw_sensor/generated/sensor_constants.h
  COMMAND python3
    ${CMAKE_CURRENT_LIST_DIR}/py/pw_sensor/sensor_desc.py
    -I $ENV{PW_ROOT}
    -g "python3 ${CMAKE_CURRENT_LIST_DIR}/py/pw_sensor/constants_generator.py --package pw.sensor"
    -o ${CMAKE_CURRENT_BINARY_DIR}/include/pw_sensor/generated/sensor_constants.h
    ${CMAKE_CURRENT_LIST_DIR}/sensor.yaml
  DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/py/pw_sensor/sensor_desc.py
    ${CMAKE_CURRENT_LIST_DIR}/py/pw_sensor/constants_generator.py
    ${CMAKE_CURRENT_LIST_DIR}/sensor.yaml
    ${CMAKE_CURRENT_LIST_DIR}/attributes.yaml
    ${CMAKE_CURRENT_LIST_DIR}/channels.yaml
    ${CMAKE_CURRENT_LIST_DIR}/triggers.yaml
  COMMENT "Generating header file..."
)

add_custom_target(pw_sensor.generate_constants
  DEPENDS
  ${CMAKE_CURRENT_BINARY_DIR}/include/pw_sensor/generated/sensor_constants.h
)

pw_add_library(pw_sensor INTERFACE
  HEADERS
    public/pw_sensor/sensor.h
    public/pw_sensor/types.h
  PUBLIC_INCLUDES
    public
    ${CMAKE_CURRENT_BINARY_DIR}/include/
  PUBLIC_DEPS
    pw_containers
    pw_tokenizer
)
target_sources(pw_sensor
  PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}/include/pw_sensor/generated/sensor_constants.h
)
set_target_properties(pw_sensor
  PROPERTIES PRIVATE_DEPENDS "pw_sensor.generate_constants"
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
  PRIVATE_DEPS
    pw_sensor
  GROUPS
    modules
    pw_sensor
)
