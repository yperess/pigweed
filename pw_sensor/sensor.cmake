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

# Create a library that exposes a generated header for all required
#
# Usage:
#  pw_sensor_library(LIBRARY_NAME
#    OUT_HEADER
#      generated/sensor_constants.h
#    SOURCES
#      bma4xx.yaml
#      bmi160.yaml
#    DEPENDS
#      $ENV{PW_ROOT}/pw_sensor/attributes.yaml
#      $ENV{PW_ROOT}/pw_sensor/channels.yaml
#      $ENV{PW_ROOT}/pw_sensor/triggers.yaml
#    INCLUDES
#      $ENV{PW_ROOT}
#  )
function(pw_sensor_library NAME)
  pw_parse_arguments(
    NUM_POSITIONAL_ARGS
      1
    MULTI_VALUE_ARGS
      DEPENDS
      INCLUDES
      SOURCES
    ONE_VALUE_ARGS
      OUT_HEADER
    REQUIRED_ARGS
      DEPENDS
      INCLUDES
      SOURCES
      OUT_HEADER
  )
  message(DEBUG "DEPENDS:    ${arg_DEPENDS}")
  message(DEBUG "INCLUDES:   ${arg_INCLUDES}")
  message(DEBUG "SOURCES:    ${arg_SOURCES}")
  message(DEBUG "OUT_HEADER: ${arg_OUT_HEADER}")

  set(output_file "${CMAKE_CURRENT_BINARY_DIR}/include/${arg_OUT_HEADER}")

  set(include_list)
  foreach(item IN LISTS arg_INCLUDES)
    list(APPEND include_list "-I" "${item}")
  endforeach()

  add_custom_command(
    OUTPUT ${output_file}
    COMMAND python3
    $ENV{PW_ROOT}/pw_sensor/py/pw_sensor/sensor_desc.py
      ${include_list}
      -g "python3 $ENV{PW_ROOT}/pw_sensor/py/pw_sensor/constants_generator.py --package pw.sensor"
      -o ${output_file}
      ${arg_SOURCES}
    DEPENDS
      $ENV{PW_ROOT}/pw_sensor/py/pw_sensor/constants_generator.py
      $ENV{PW_ROOT}/pw_sensor/py/pw_sensor/sensor_desc.py
      ${arg_DEPENDS}
      ${arg_SOURCES}
  )
  add_custom_target(${NAME}.__generate_constants
    DEPENDS
    ${output_file}
  )
  pw_add_library(${NAME} INTERFACE
    HEADERS
      $ENV{PW_ROOT}/pw_sensor/public/pw_sensor/types.h
    PUBLIC_INCLUDES
      public
      ${CMAKE_CURRENT_BINARY_DIR}/include/
    PUBLIC_DEPS
      pw_containers
      pw_tokenizer
  )
  target_sources(${NAME} PUBLIC ${output_file})
  set_target_properties(${NAME}
    PROPERTIES PRIVATE_DEPENDS ${NAME}.__generate_constants
  )
endfunction(pw_sensor_library)

