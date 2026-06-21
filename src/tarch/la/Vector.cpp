// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Vector.h"

#if defined(PEANO_WITH_GPU_BACKEND_SYCL)
#include <sycl/sycl.hpp>
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

#if defined(PEANO_WITH_GPU_BACKEND_SYCL)
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
