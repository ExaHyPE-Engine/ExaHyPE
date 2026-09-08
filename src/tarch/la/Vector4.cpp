// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/la/Vector.h"

static_assert(sizeof(tarch::la::Vector<4, int>) == 16, "Vector<4,int> should stay 16 bytes");
static_assert(alignof(tarch::la::Vector<4, int>) == 16, "Vector<4,int> should be 16-byte aligned");
static_assert(std::is_standard_layout<tarch::la::Vector<4, int>>::value, "Vector<4,int> should be standard-layout");
static_assert(
  std::is_trivially_copyable<tarch::la::Vector<4, int>>::value,
  "Vector<4,int> should be trivially copyable"
);
static_assert(sizeof(tarch::la::Vector<4, double>) == 32, "Vector<4,double> should stay 32 bytes");
