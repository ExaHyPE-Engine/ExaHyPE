// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TestDataOnStack.h"

#include "tarch/accelerator/accelerator.h"

peano4::stacks::tests::TestDataOnStack::TestDataOnStack() { data.fill(0); }

peano4::stacks::tests::TestDataOnStack::TestDataOnStack(int v) { data.fill(v); }

peano4::stacks::tests::TestDataOnStack::TestDataOnStack(const TestDataOnStack& other) { clone(other); }

peano4::stacks::tests::TestDataOnStack::TestDataOnStack(TestDataOnStack&& other) noexcept {
  data = std::move(other.data);
}

peano4::stacks::tests::TestDataOnStack&
peano4::stacks::tests::TestDataOnStack::operator=(TestDataOnStack&& other) noexcept {
  if (this != &other) {
    data = std::move(other.data);
  }
  return *this;
}

void peano4::stacks::tests::TestDataOnStack::clone(const TestDataOnStack& other) {
  std::copy(std::begin(other.data), std::end(other.data), std::begin(data));
}
