// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TestDataOnHeap.h"

#include "tarch/Assertions.h"
#include "tarch/accelerator/accelerator.h"

#include <cstring> // std::memcpy
#include <utility> // std::exchange

peano4::stacks::tests::TestDataOnHeap::TestDataOnHeap():
  _data(tarch::allocateMemory<DoFType>(Cardinality, tarch::MemoryLocation::Heap)) {}

peano4::stacks::tests::TestDataOnHeap::TestDataOnHeap(DoFType v):
  TestDataOnHeap() {

  for (int i = 0; i < Cardinality; ++i) {
    _data[i] = v;
  }
}

peano4::stacks::tests::TestDataOnHeap::~TestDataOnHeap() noexcept {
  if (_data) {
    tarch::freeMemory(_data, tarch::MemoryLocation::Heap);
    _data = nullptr;
  }
}

peano4::stacks::tests::TestDataOnHeap::TestDataOnHeap(const TestDataOnHeap& other):
  _data(nullptr) {
  clone(other);
}

peano4::stacks::tests::TestDataOnHeap::TestDataOnHeap(TestDataOnHeap&& other) noexcept:
  _data(std::exchange(other._data, nullptr)) {}

peano4::stacks::tests::TestDataOnHeap& peano4::stacks::tests::TestDataOnHeap::operator=(
  TestDataOnHeap&& other
) noexcept {
  if (this != &other) {
    if (_data) {
      tarch::freeMemory(_data, tarch::MemoryLocation::Heap);
    }
    _data = std::exchange(other._data, nullptr);
  }
  return *this;
}

peano4::stacks::tests::TestDataOnHeap::DoFType* peano4::stacks::tests::TestDataOnHeap::data() { return _data; }

const peano4::stacks::tests::TestDataOnHeap::DoFType* peano4::stacks::tests::TestDataOnHeap::data() const {
  return _data;
}

void peano4::stacks::tests::TestDataOnHeap::clone(const TestDataOnHeap& other) {
  if (this == &other)
    return;

  if (other._data == nullptr) {
    // other has no _data: just reset
    if (_data) {
      tarch::freeMemory(_data, tarch::MemoryLocation::Heap);
      _data = nullptr;
    }
    return;
  }

  DoFType* newbuf = tarch::allocateMemory<DoFType>(Cardinality, tarch::MemoryLocation::Heap);
  assertion(newbuf);
  std::memcpy(newbuf, other._data, Cardinality * sizeof(DoFType));

  // Commit
  if (_data) {
    tarch::freeMemory(_data, tarch::MemoryLocation::Heap);
  }
  _data = newbuf;
}

#ifdef WITH_MPI
MPI_Datatype peano4::stacks::tests::TestDataOnHeap::getBoundaryExchangeDatatype() { return MPI_INT; }
MPI_Datatype peano4::stacks::tests::TestDataOnHeap::getMultiscaleDataExchangeDatatype() { return MPI_INT; }
MPI_Datatype peano4::stacks::tests::TestDataOnHeap::getForkDatatype() { return MPI_INT; }
MPI_Datatype peano4::stacks::tests::TestDataOnHeap::getJoinDatatype() { return MPI_INT; }
MPI_Datatype peano4::stacks::tests::TestDataOnHeap::getGlobalCommunicationDatatype() { return MPI_INT; }
#endif
