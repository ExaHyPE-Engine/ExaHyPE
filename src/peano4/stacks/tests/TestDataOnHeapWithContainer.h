// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/stacks/Stack.h"

#include <vector>

#ifdef PEANO_WITH_MPI
#include <mpi.h>
#endif

namespace peano4::stacks::tests {
  class TestDataOnHeapWithContainer;
} // namespace peano4::stacks::tests

class peano4::stacks::tests::TestDataOnHeapWithContainer {
public:
  using DoFType = int;

private:
  std::vector<DoFType*> _container;

public:
  explicit TestDataOnHeapWithContainer();
  explicit TestDataOnHeapWithContainer(std::size_t size, DoFType v);
  ~TestDataOnHeapWithContainer() noexcept;

  TestDataOnHeapWithContainer(const TestDataOnHeapWithContainer& other);
  TestDataOnHeapWithContainer(TestDataOnHeapWithContainer&& other) noexcept;
  TestDataOnHeapWithContainer& operator=(const TestDataOnHeapWithContainer& other) = delete;
  TestDataOnHeapWithContainer& operator=(TestDataOnHeapWithContainer&& other) noexcept;

  DoFType at(int index);
  int     size() const;

  void clear();
  void clone(const TestDataOnHeapWithContainer& other);

#ifdef PEANO_WITH_MPI
public:
  static MPI_Datatype getBoundaryExchangeDatatype();
  static MPI_Datatype getMultiscaleDataExchangeDatatype();
  static MPI_Datatype getForkDatatype();
  static MPI_Datatype getJoinDatatype();
  static MPI_Datatype getGlobalCommunicationDatatype();

  static void startSend(Stack<TestDataOnHeapWithContainer>& stack, const SendReceiveContext context, MPI_Comm comm);

  static void startReceive(
    Stack<TestDataOnHeapWithContainer>& stack,
    const SendReceiveContext            context,
    MPI_Comm                            comm,
    int                                 numberOfElements
  );

  static bool tryToFinishSendOrReceive(Stack<TestDataOnHeapWithContainer>& stack);
#endif
};
