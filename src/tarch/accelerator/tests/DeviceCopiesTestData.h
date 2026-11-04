// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch::accelerator::tests {
  inline double hostValue{0.0};

  double getValueOfAKernelOfTheTestData();
} // namespace tarch::accelerator::tests

#include "tarch/accelerator/tests/DeviceCopiesTestData.cpph"
