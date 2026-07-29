// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace peano4::stacks::tests {
  class TestDataOnHeap;
} // namespace peano4::stacks::tests

class peano4::stacks::tests::TestDataOnHeap {
public:
  using DoFType = int;

  static constexpr int Cardinality = 1024;

private:
  DoFType* _data;

public:
  explicit TestDataOnHeap();
  explicit TestDataOnHeap(DoFType v);
  ~TestDataOnHeap() noexcept;

  TestDataOnHeap(const TestDataOnHeap& other);
  TestDataOnHeap(TestDataOnHeap&& other) noexcept;
  TestDataOnHeap& operator=(const TestDataOnHeap& other) = delete;
  TestDataOnHeap& operator=(TestDataOnHeap&& other) noexcept;

  DoFType*       data();
  const DoFType* data() const;

  void clone(const TestDataOnHeap& other);
};
