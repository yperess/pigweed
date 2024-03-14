// Copyright 2023 The Pigweed Authors
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

#include <cstddef>

#include "pw_allocator/allocator.h"
#include "pw_allocator/testing.h"
#include "pw_unit_test/framework.h"

namespace pw::allocator {
namespace {

TEST(UniquePtr, DefaultInitializationIsNullptr) {
  UniquePtr<int> empty;
  EXPECT_EQ(empty.get(), nullptr);
}

TEST(UniquePtr, OperatorEqNullptrOnEmptyUniquePtrSucceeds) {
  UniquePtr<int> empty;
  EXPECT_TRUE(empty == nullptr);
  EXPECT_FALSE(empty != nullptr);
}

TEST(UniquePtr, OperatorEqNullptrAfterMakeUniqueFails) {
  test::AllocatorForTest<256> allocator;
  std::optional<UniquePtr<int>> ptr_opt = allocator.MakeUnique<int>(5);
  ASSERT_TRUE(ptr_opt.has_value());
  UniquePtr<int> ptr = std::move(*ptr_opt);
  EXPECT_TRUE(ptr != nullptr);
  EXPECT_FALSE(ptr == nullptr);
}

TEST(UniquePtr, OperatorEqNullptrAfterMakeUniqueNullptrTypeFails) {
  test::AllocatorForTest<256> allocator;
  std::optional<UniquePtr<std::nullptr_t>> ptr_opt =
      allocator.MakeUnique<std::nullptr_t>(nullptr);
  ASSERT_TRUE(ptr_opt.has_value());
  UniquePtr<std::nullptr_t> ptr = std::move(*ptr_opt);
  EXPECT_TRUE(ptr != nullptr);
  EXPECT_FALSE(ptr == nullptr);
  EXPECT_TRUE(*ptr == nullptr);
  EXPECT_FALSE(*ptr != nullptr);
}

TEST(UniquePtr, MakeUniqueForwardsConstructorArguments) {
  class MoveOnly {
   public:
    MoveOnly(int value) : value_(value) {}
    MoveOnly(MoveOnly&) = delete;
    MoveOnly(MoveOnly&&) {}
    int Value() const { return value_; }

   private:
    int value_;
  };

  class BuiltWithMoveOnly {
   public:
    BuiltWithMoveOnly() = delete;
    BuiltWithMoveOnly(MoveOnly&& mo) : value_(mo.Value()) {}
    int Value() const { return value_; }

   private:
    int value_;
  };

  test::AllocatorForTest<256> allocator;
  MoveOnly mo(6);
  std::optional<UniquePtr<BuiltWithMoveOnly>> ptr =
      allocator.MakeUnique<BuiltWithMoveOnly>(std::move(mo));
  ASSERT_TRUE(ptr.has_value());
  EXPECT_EQ((*ptr)->Value(), 6);
}

TEST(UniquePtr, MoveConstructsFromSubClassAndFreesTotalSize) {
  struct Base {};
  struct LargerSub : public Base {
    std::array<std::byte, 128> mem;
  };
  test::AllocatorForTest<256> allocator;
  std::optional<UniquePtr<LargerSub>> ptr_opt =
      allocator.MakeUnique<LargerSub>();
  ASSERT_TRUE(ptr_opt.has_value());
  EXPECT_EQ(allocator.allocate_size(), 128ul);
  UniquePtr<LargerSub> ptr = std::move(*ptr_opt);
  UniquePtr<Base> base_ptr(std::move(ptr));

  EXPECT_EQ(allocator.deallocate_size(), 0ul);
  // The size that is deallocated here should be the size of the larger
  // subclass, not the size of the smaller base class.
  base_ptr.Reset();
  EXPECT_EQ(allocator.deallocate_size(), 128ul);
}

TEST(UniquePtr, MoveAssignsFromSubClassAndFreesTotalSize) {
  struct Base {};
  struct LargerSub : public Base {
    std::array<std::byte, 128> mem;
  };
  test::AllocatorForTest<256> allocator;
  std::optional<UniquePtr<LargerSub>> ptr_opt =
      allocator.MakeUnique<LargerSub>();
  ASSERT_TRUE(ptr_opt.has_value());
  EXPECT_EQ(allocator.allocate_size(), 128ul);
  UniquePtr<LargerSub> ptr = std::move(*ptr_opt);
  UniquePtr<Base> base_ptr = std::move(ptr);

  EXPECT_EQ(allocator.deallocate_size(), 0ul);
  // The size that is deallocated here should be the size of the larger
  // subclass, not the size of the smaller base class.
  base_ptr.Reset();
  EXPECT_EQ(allocator.deallocate_size(), 128ul);
}

TEST(UniquePtr, MoveAssignsToExistingDeallocates) {
  test::AllocatorForTest<256> allocator;
  std::optional<UniquePtr<size_t>> result = allocator.MakeUnique<size_t>(1);
  ASSERT_TRUE(result.has_value());
  UniquePtr<size_t> size1 = std::move(result.value());

  EXPECT_EQ(*size1, 1U);

  result = allocator.MakeUnique<size_t>(2);
  ASSERT_TRUE(result.has_value());
  UniquePtr<size_t> size2 = std::move(result.value());
  EXPECT_EQ(*size2, 2U);

  EXPECT_EQ(allocator.deallocate_size(), 0U);

  size1 = std::move(size2);
  EXPECT_EQ(allocator.deallocate_size(), sizeof(size_t));
  EXPECT_EQ(*size1, 2U);
}

TEST(UniquePtr, DestructorDestroysAndFrees) {
  int count = 0;
  class DestructorCounter {
   public:
    DestructorCounter(int& count) : count_(&count) {}
    ~DestructorCounter() { (*count_)++; }

   private:
    int* count_;
  };
  test::AllocatorForTest<256> allocator;
  std::optional<UniquePtr<DestructorCounter>> ptr_opt =
      allocator.MakeUnique<DestructorCounter>(count);
  ASSERT_TRUE(ptr_opt.has_value());

  EXPECT_EQ(count, 0);
  EXPECT_EQ(allocator.deallocate_size(), 0ul);
  ptr_opt.reset();  // clear the optional, destroying the UniquePtr
  EXPECT_EQ(count, 1);
  EXPECT_EQ(allocator.deallocate_size(), sizeof(DestructorCounter));
}

}  // namespace
}  // namespace pw::allocator
