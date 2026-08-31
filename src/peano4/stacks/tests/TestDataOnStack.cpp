// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TestDataOnStack.h"

peano4::stacks::tests::TestDataOnStack::TestDataOnStack(const int value) { data.fill(value); }

peano4::stacks::tests::TestDataOnStack::TestDataOnStack(const peano4::stacks::tests::TestDataOnStack& other) {
  clone(other);
}

peano4::stacks::tests::TestDataOnStack::TestDataOnStack(peano4::stacks::tests::TestDataOnStack&& other) noexcept {
  data = std::move(other.data);
}

peano4::stacks::tests::TestDataOnStack& peano4::stacks::tests::TestDataOnStack::operator=(
  peano4::stacks::tests::TestDataOnStack&& other
) noexcept {
  if (this != &other) {
    data = std::move(other.data);
  }
  return *this;
}

void peano4::stacks::tests::TestDataOnStack::clone(const peano4::stacks::tests::TestDataOnStack& other) {
  std::copy(std::begin(other.data), std::end(other.data), std::begin(data));
}
