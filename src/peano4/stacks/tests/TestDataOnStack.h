// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace peano4::stacks::tests {
  class TestDataOnStack;
} // namespace peano4::stacks::tests

class peano4::stacks::tests::TestDataOnStack {
public:
  static constexpr int Cardinality = 1024;

  std::array<int, Cardinality> data;

  explicit TestDataOnStack();
  explicit TestDataOnStack(int v);
  ~TestDataOnStack() noexcept = default;

  TestDataOnStack(const TestDataOnStack& other);
  TestDataOnStack(TestDataOnStack&& other) noexcept;
  TestDataOnStack& operator=(const TestDataOnStack& other) = delete;
  TestDataOnStack& operator=(TestDataOnStack&& other) noexcept;

  void clone(const TestDataOnStack& other);
};
