// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/stacks/Stack.h"

#include <mpi.h>

namespace peano4::stacks::tests {
  class TestDataOnHeapWithContainer;
} // namespace peano4::stacks::tests

class peano4::stacks::tests::TestDataOnHeapWithContainer {
public:
  using DoFType = int;

  TestDataOnHeapWithContainer() = default;
  explicit TestDataOnHeapWithContainer(std::size_t size, DoFType value);
  ~TestDataOnHeapWithContainer() noexcept;

  TestDataOnHeapWithContainer(const TestDataOnHeapWithContainer& other);
  TestDataOnHeapWithContainer(TestDataOnHeapWithContainer&& other) noexcept;
  TestDataOnHeapWithContainer& operator=(const TestDataOnHeapWithContainer& other) = delete;
  TestDataOnHeapWithContainer& operator=(TestDataOnHeapWithContainer&& other) noexcept;

  DoFType at(int index) const;
  int     size() const;

  void clear();
  void clone(const TestDataOnHeapWithContainer& other);

  static void startSend(Stack<TestDataOnHeapWithContainer>& stack, const SendReceiveContext context, MPI_Comm comm);

  static void startReceive(
    Stack<TestDataOnHeapWithContainer>& stack,
    const SendReceiveContext            context,
    MPI_Comm                            comm,
    int                                 numberOfElements
  );

  static bool tryToFinishSendOrReceive(Stack<TestDataOnHeapWithContainer>& stack);

private:
  std::vector<DoFType*> _container{};
};
