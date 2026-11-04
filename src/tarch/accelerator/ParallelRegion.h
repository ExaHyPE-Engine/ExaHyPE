// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <omp.h>

namespace tarch::accelerator {
  template <typename Body>
  inline void runInParallelRegion(Body&& body) {
#pragma omp parallel default(none) shared(body) if (not omp_in_parallel())
    body();
  }
} // namespace tarch::accelerator
