// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch::accelerator::unittests {
  inline double hostValue{0.0};

  double getValueOfAKernelOfTheTestData();
} // namespace tarch::accelerator::unittests

#include "unittests/tarch/accelerator/DeviceCopiesTestData.cpph"
