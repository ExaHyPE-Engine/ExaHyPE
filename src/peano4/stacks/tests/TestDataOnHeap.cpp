// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TestDataOnHeap.h"

#include "tarch/Assertions.h"
#include "tarch/accelerator/accelerator.h"

#include <cstring>

peano4::stacks::tests::TestDataOnHeap::TestDataOnHeap():
  value(tarch::allocateMemory<DoFType>(Cardinality, tarch::MemoryLocation::Heap)) {}

peano4::stacks::tests::TestDataOnHeap::TestDataOnHeap(DoFType v):
  value(tarch::allocateMemory<DoFType>(Cardinality, tarch::MemoryLocation::Heap)) {

  for (int i = 0; i < Cardinality; ++i) {
    value[i] = v;
  }
}

peano4::stacks::tests::TestDataOnHeap::~TestDataOnHeap() {
  if (value) {
    tarch::freeMemory(value, tarch::MemoryLocation::Heap);
  }
}

peano4::stacks::tests::TestDataOnHeap::TestDataOnHeap(const TestDataOnHeap& other):
  value(nullptr) {
  clone(other);
}

peano4::stacks::tests::TestDataOnHeap::TestDataOnHeap(TestDataOnHeap&& other) noexcept:
  value(other.value) {
  other.value = nullptr;
}

peano4::stacks::tests::TestDataOnHeap& peano4::stacks::tests::TestDataOnHeap::operator=(TestDataOnHeap&& other
) noexcept {
  if (this != &other) {
    if (value) {
      tarch::freeMemory(value, tarch::MemoryLocation::Heap);
    }
    value       = other.value;
    other.value = nullptr;
  }
  return *this;
}

void peano4::stacks::tests::TestDataOnHeap::clone(const TestDataOnHeap& other) {
  if (value) {
    tarch::freeMemory(value, tarch::MemoryLocation::Heap);
    value = nullptr;
  }

  if (other.value == nullptr) {
    value = nullptr;
  } else {
    value = tarch::allocateMemory<DoFType>(Cardinality, tarch::MemoryLocation::Heap);
    assertion(value);
    assertion(other.value);
    std::copy_n(other.value, Cardinality, value);
  }
}

#ifdef WITH_MPI
MPI_Datatype peano4::stacks::tests::TestDataOnHeap::getBoundaryExchangeDatatype() { return MPI_INT; }
MPI_Datatype peano4::stacks::tests::TestDataOnHeap::getMultiscaleDataExchangeDatatype() { return MPI_INT; }
MPI_Datatype peano4::stacks::tests::TestDataOnHeap::getForkDatatype() { return MPI_INT; }
MPI_Datatype peano4::stacks::tests::TestDataOnHeap::getJoinDatatype() { return MPI_INT; }
MPI_Datatype peano4::stacks::tests::TestDataOnHeap::getGlobalCommunicationDatatype() { return MPI_INT; }
#endif
