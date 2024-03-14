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

#include "pw_allocator/null_allocator.h"

#include "pw_allocator/size_reporter.h"

namespace pw::allocator {

void Run() {
  SizeReporter size_reporter;
  NullAllocator allocator;
  size_reporter.MeasureAllocator(&allocator);
}

}  // namespace pw::allocator

int main() {
  pw::allocator::Run();
  return 0;
}
