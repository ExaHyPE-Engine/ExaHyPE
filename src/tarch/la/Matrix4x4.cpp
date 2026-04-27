// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Matrix.h"

static_assert(sizeof(tarch::la::Matrix<4, 4, int>) == 64, "Matrix<4,4,int> should remain contiguous");
static_assert(alignof(tarch::la::Matrix<4, 4, int>) == 16, "Matrix<4,4,int> should be 16-byte aligned");
static_assert(
  std::is_standard_layout<tarch::la::Matrix<4, 4, int>>::value,
  "Matrix<4,4,int> should be standard-layout"
);
static_assert(
  std::is_trivially_copyable<tarch::la::Matrix<4, 4, int>>::value,
  "Matrix<4,4,int> should be trivially copyable"
);
