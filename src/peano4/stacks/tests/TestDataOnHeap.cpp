// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TestDataOnHeap.h"

#include "tarch/Assertions.h"
#include "tarch/accelerator/accelerator.h"

peano4::stacks::tests::TestDataOnHeap::TestDataOnHeap() :
  _data(tarch::allocateMemory<DoFType>(Cardinality, tarch::MemoryLocation::Heap)) {}

peano4::stacks::tests::TestDataOnHeap::TestDataOnHeap(const DoFType value) :
  peano4::stacks::tests::TestDataOnHeap() {
  for (int index = 0; index < Cardinality; ++index) {
    _data[index] = value;
  }
}

peano4::stacks::tests::TestDataOnHeap::TestDataOnHeap(const peano4::stacks::tests::TestDataOnHeap& other) {
  clone(other);
}

peano4::stacks::tests::TestDataOnHeap::TestDataOnHeap(peano4::stacks::tests::TestDataOnHeap&& other) noexcept :
  _data(std::exchange(other._data, nullptr)) {}

peano4::stacks::tests::TestDataOnHeap& peano4::stacks::tests::TestDataOnHeap::operator=(
  peano4::stacks::tests::TestDataOnHeap&& other
) noexcept {
  if (this != &other) {
    if (_data != nullptr) {
      tarch::freeMemory(_data, tarch::MemoryLocation::Heap);
    }
    _data = std::exchange(other._data, nullptr);
  }
  return *this;
}

peano4::stacks::tests::TestDataOnHeap::~TestDataOnHeap() noexcept {
  if (_data != nullptr) {
    tarch::freeMemory(_data, tarch::MemoryLocation::Heap);
    _data = nullptr;
  }
}

peano4::stacks::tests::TestDataOnHeap::DoFType* peano4::stacks::tests::TestDataOnHeap::data() { return _data; }

const peano4::stacks::tests::TestDataOnHeap::DoFType* peano4::stacks::tests::TestDataOnHeap::data() const {
  return _data;
}

void peano4::stacks::tests::TestDataOnHeap::clone(const peano4::stacks::tests::TestDataOnHeap& other) {
  if (this == &other) {
    return;
  }

  if (other._data == nullptr) {
    if (_data != nullptr) {
      tarch::freeMemory(_data, tarch::MemoryLocation::Heap);
      _data = nullptr;
    }
    return;
  }

  DoFType* const clonedData = tarch::allocateMemory<DoFType>(Cardinality, tarch::MemoryLocation::Heap);
  assertion(clonedData != nullptr);
  std::memcpy(clonedData, other._data, Cardinality * sizeof(DoFType));

  // The held buffer is released only once the copy exists, so a failed
  // allocation leaves the held buffer in place.
  if (_data != nullptr) {
    tarch::freeMemory(_data, tarch::MemoryLocation::Heap);
  }
  _data = clonedData;
}
