// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Vector.h"

static_assert(sizeof(tarch::la::Vector<3, int>) == 12, "Vector<3,int> should stay tightly packed");
static_assert(alignof(tarch::la::Vector<3, int>) == alignof(int), "Vector<3,int> should use natural alignment");
static_assert(std::is_standard_layout<tarch::la::Vector<3, int>>::value, "Vector<3,int> should be standard-layout");
static_assert(
  std::is_trivially_copyable<tarch::la::Vector<3, int>>::value,
  "Vector<3,int> should be trivially copyable"
);
static_assert(sizeof(tarch::la::Vector<3, double>) == 24, "Vector<3,double> should stay tightly packed");
