// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(PEANO_WITH_GPU_BACKEND_SYCL)

#include <sycl/sycl.hpp>

namespace tarch::accelerator {
  /**
   * Holds the SYCL work item of the running kernel. The item exists for the whole kernel, so a
   * nested call may copy this handle by value. The scratch is work-group local memory for custom
   * team reductions.
   */
  struct SyclTeam final {
    const ::sycl::nd_item<1>& item;
    unsigned char*            scratch{nullptr};
  };
  using TeamHandle = SyclTeam;

  inline constexpr int SyclLocalSize        = 256;
  inline constexpr int SyclTeamScratchBytes = 16384;

  void* getSYCLQueue(int number);
} // namespace tarch::accelerator

#endif
