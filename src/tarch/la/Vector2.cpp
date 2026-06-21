// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Vector.h"

#if defined(PEANO_WITH_GPU_BACKEND_SYCL)
#include <sycl/sycl.hpp>
#endif

static_assert(sizeof(tarch::la::Vector<2, int>) == 8, "Vector<2,int> should stay tightly packed");
static_assert(alignof(tarch::la::Vector<2, int>) == 8, "Vector<2,int> should be 8-byte aligned");
static_assert(std::is_standard_layout<tarch::la::Vector<2, int>>::value, "Vector<2,int> should be standard-layout");
static_assert(
  std::is_trivially_copyable<tarch::la::Vector<2, int>>::value,
  "Vector<2,int> should be trivially copyable"
);
static_assert(sizeof(tarch::la::Vector<2, double>) == 16, "Vector<2,double> should stay tightly packed");

#if defined(PEANO_WITH_GPU_BACKEND_SYCL)
static_assert(
  sycl::is_device_copyable<tarch::la::Vector<2, int>>::value,
  "Vector<2,int> should be SYCL device copyable"
);
static_assert(
  sycl::is_device_copyable<tarch::la::Vector<2, double>>::value,
  "Vector<2,double> should be SYCL device copyable"
);
#endif
