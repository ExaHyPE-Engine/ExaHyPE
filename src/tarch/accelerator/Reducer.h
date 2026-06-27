// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/accelerator.h"

#include <limits>

namespace tarch::accelerator {
  /**
   * Reducer wrappers for tarch::parallelReduce.
   *
   * A reducer carries a reference to the user's accumulator variable plus the
   * reduction operation (max/min/sum). Each backend specialises how to express
   * the operation: OMP `reduction(...)` clause, Kokkos::Max/Min/Sum, sycl::reduction,
   * shmem-reduce + cub::DeviceReduce, etc.
   *
   * The reducer's value_type is what the inner lambda receives as a thread-local
   * accumulator parameter, e.g.
   *   parallelReduce(range, [=](int i, double& localMax){...}, Max<double>{result});
   *
   * Identity values are exposed so backends that need to seed thread-local
   * accumulators can do so consistently.
   */

  template <typename T>
  struct Max {
    using value_type = T;
    T& target;

    static GPU_CALLABLE_METHOD constexpr T identity() { return std::numeric_limits<T>::lowest(); }
  };

  template <typename T>
  struct Min {
    using value_type = T;
    T& target;

    static GPU_CALLABLE_METHOD constexpr T identity() { return std::numeric_limits<T>::max(); }
  };

  template <typename T>
  struct Sum {
    using value_type = T;
    T& target;

    static GPU_CALLABLE_METHOD constexpr T identity() { return T{0}; }
  };
} // namespace tarch::accelerator

namespace tarch {
  using accelerator::Max;
  using accelerator::Min;
  using accelerator::Sum;
} // namespace tarch
