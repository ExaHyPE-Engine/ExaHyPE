// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/la/Matrix.h"

static_assert(sizeof(tarch::la::Matrix<3, 3, int>) == 48, "Matrix<3,3,int> should remain contiguous");
static_assert(alignof(tarch::la::Matrix<3, 3, int>) == 16, "Matrix<3,3,int> should be 16-byte aligned");
static_assert(
  std::is_standard_layout<tarch::la::Matrix<3, 3, int>>::value,
  "Matrix<3,3,int> should be standard-layout"
);
static_assert(
  std::is_trivially_copyable<tarch::la::Matrix<3, 3, int>>::value,
  "Matrix<3,3,int> should be trivially copyable"
);
