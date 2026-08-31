// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace peano4::stacks::tests {
  class TestDataOnStack;
} // namespace peano4::stacks::tests

class peano4::stacks::tests::TestDataOnStack final {
public:
  static constexpr int Cardinality = 1024;

  std::array<int, Cardinality> data{};

  TestDataOnStack() = default;
  explicit TestDataOnStack(const int value);

  TestDataOnStack(const TestDataOnStack& other);
  TestDataOnStack(TestDataOnStack&& other) noexcept;
  TestDataOnStack& operator=(const TestDataOnStack& other) = delete;
  TestDataOnStack& operator=(TestDataOnStack&& other) noexcept;
  ~TestDataOnStack() noexcept = default;

  void clone(const TestDataOnStack& other);
};
