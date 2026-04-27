// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Matrix.h"

static_assert(sizeof(tarch::la::Matrix<1, 1, int>) == sizeof(int), "Generic matrix should remain contiguous");
static_assert(
  std::is_trivially_copyable<tarch::la::Matrix<1, 1, int>>::value,
  "Generic matrix should be trivially copyable"
);
