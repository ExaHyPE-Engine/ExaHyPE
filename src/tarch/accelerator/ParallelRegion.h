// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(_OPENMP)
#include <omp.h>
#endif

namespace tarch::accelerator {
  template <typename Body>
  inline void run_in_parallel_region(Body&& body) {
#if defined(_OPENMP)
#pragma omp parallel if (!omp_in_parallel())
    body();
#else
    body();
#endif
  }
} // namespace tarch::accelerator
