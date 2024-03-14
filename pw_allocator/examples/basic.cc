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

#include "examples/named_u32.h"
#include "pw_allocator/allocator.h"
#include "pw_allocator/testing.h"
#include "pw_unit_test/framework.h"

namespace examples {

// DOCSTAG: [pw_allocator-examples-basic-allocate]
using pw::allocator::Allocator;
using pw::allocator::Layout;

void* AllocateNamedU32(Allocator& allocator) {
  return allocator.Allocate(Layout::Of<NamedU32>());
}
// DOCSTAG: [pw_allocator-examples-basic-allocate]

// DOCSTAG: [pw_allocator-examples-basic-deallocate]
void DeallocateNamedU32(Allocator& allocator, void* ptr) {
  allocator.Deallocate(ptr, Layout::Of<NamedU32>());
}
// DOCSTAG: [pw_allocator-examples-basic-deallocate]

// DOCSTAG: [pw_allocator-examples-basic-new_delete]
NamedU32* NewNamedU32(Allocator& allocator,
                      std::string_view name,
                      uint32_t value) {
  return allocator.New<NamedU32>(name, value);
}

void DeleteNamedU32(Allocator& allocator, NamedU32* named_u32) {
  allocator.Delete<NamedU32>(named_u32);
}
// DOCSTAG: [pw_allocator-examples-basic-new_delete]

// DOCSTAG: [pw_allocator-examples-basic-make_unique]
std::optional<pw::allocator::UniquePtr<NamedU32>> MakeNamedU32(
    Allocator& allocator, std::string_view name, uint32_t value) {
  return allocator.MakeUnique<NamedU32>(name, value);
}
// DOCSTAG: [pw_allocator-examples-basic-make_unique]

}  // namespace examples

namespace pw::allocator {

TEST(BasicExample, AllocateNamedU32) {
  test::AllocatorForTest<256> allocator;
  void* ptr = examples::AllocateNamedU32(allocator);
  ASSERT_NE(ptr, nullptr);
  examples::DeallocateNamedU32(allocator, ptr);
}

TEST(BasicExample, NewNamedU32) {
  test::AllocatorForTest<256> allocator;
  auto* named_u32 = examples::NewNamedU32(allocator, "test1", 111);
  ASSERT_NE(named_u32, nullptr);
  EXPECT_STREQ(named_u32->name().data(), "test1");
  EXPECT_EQ(named_u32->value(), 111U);
  examples::DeleteNamedU32(allocator, named_u32);
}

TEST(BasicExample, MakeNamedU32) {
  test::AllocatorForTest<256> allocator;
  auto result = examples::MakeNamedU32(allocator, "test2", 222);
  ASSERT_TRUE(result.has_value());
  UniquePtr<examples::NamedU32> named_u32 = std::move(result.value());
  EXPECT_STREQ(named_u32->name().data(), "test2");
  EXPECT_EQ(named_u32->value(), 222U);
}

}  // namespace pw::allocator
