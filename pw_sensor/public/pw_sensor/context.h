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

#include "pw_containers/intrusive_list.h"
// #include "pw_status/status.h"
#include "pw_sensor/internal/future.h"

namespace pw::sensor {

class SensorContextBase {
 public:
  void AddFuture(internal::Future& future) { futures_.push_back(future); }

 private:
  pw::IntrusiveList<internal::Future> futures_;
};
}  // namespace pw::sensor
