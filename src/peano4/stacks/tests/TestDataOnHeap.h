// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#ifdef WITH_MPI
#include <mpi.h>
#endif

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

#ifdef WITH_MPI
  static MPI_Datatype getBoundaryExchangeDatatype();
  static MPI_Datatype getMultiscaleDataExchangeDatatype();
  static MPI_Datatype getForkDatatype();
  static MPI_Datatype getJoinDatatype();
  static MPI_Datatype getGlobalCommunicationDatatype();
#endif
};
