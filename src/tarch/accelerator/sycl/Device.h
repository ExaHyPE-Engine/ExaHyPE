// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(PEANO_WITH_GPU_BACKEND_SYCL)

#include <sycl/sycl.hpp>

namespace tarch::accelerator {
  /**
   * Opaque team handle for a SYCL work item. The reference stays valid for the enclosing kernel
   * invocation, so a nested call may capture this handle by value.
   */
  struct SyclTeam {
    // The global qualifier avoids collision with tarch::accelerator::sycl.
    const ::sycl::nd_item<1>& item;
  };
  using TeamHandle = SyclTeam;

  inline constexpr int SyclLocalSize = 256;

  /**
   * Returns the queue of a logical device index. That index counts the accelerators only and does
   * not match a physical device number.
   */
  void* getSYCLQueue(int number);
} // namespace tarch::accelerator

#endif
