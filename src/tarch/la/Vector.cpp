// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Vector.h"

#if defined(SharedSYCL) or defined(GPUOffloadingSYCL)
#pragma push_macro("Dimensions")
#undef Dimensions
#include <sycl/sycl.hpp>
#pragma pop_macro("Dimensions")
#endif

static_assert(
  std::is_trivially_copyable<tarch::la::Vector<1, int>>::value,
  "Vector should be trivially copyable!"
);

static_assert(
  std::is_trivially_copyable<tarch::la::Vector<2, int>>::value,
  "Vector should be trivially copyable!"
);

static_assert(
  std::is_trivially_copyable<tarch::la::Vector<1, double>>::value,
  "Vector should be trivially copyable!"
);

static_assert(
  std::is_trivially_copyable<tarch::la::Vector<2, double>>::value,
  "Vector should be trivially copyable!"
);

#if defined(SharedSYCL) or defined(GPUOffloadingSYCL)
static_assert(
  sycl::is_device_copyable<tarch::la::Vector<1, int>>::value,
  "Vector should be SYCL device copyable!"
);
static_assert(
  sycl::is_device_copyable<tarch::la::Vector<2, int>>::value,
  "Vector should be SYCL device copyable!"
);
static_assert(
  sycl::is_device_copyable<tarch::la::Vector<1, double>>::value,
  "Vector should be SYCL device copyable!"
);
static_assert(
  sycl::is_device_copyable<tarch::la::Vector<2, double>>::value,
  "Vector should be SYCL device copyable!"
);
#endif
