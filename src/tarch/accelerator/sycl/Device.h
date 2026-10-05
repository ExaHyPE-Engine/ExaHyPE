// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(PEANO_WITH_GPU_BACKEND_SYCL)

#include "tarch/CompilerSpecificSettings.h"

#include <sycl/sycl.hpp>

namespace tarch::accelerator {
  /**
   * References the SYCL work item of the running kernel. The item exists for the whole kernel, so a
   * nested call may copy this handle by value. The scratch is work-group local memory for custom
   * team reductions.
   */
  struct SyclTeam final {
    const ::sycl::nd_item<1>& item;
    unsigned char*            scratch{nullptr};
  };
  using TeamHandle = SyclTeam;

  inline constexpr int SyclLocalSize  = 256;
  inline constexpr int ThreadsPerWarp = 32;
  // A team launch runs one work group per team of at most SyclLocalSize work items with at most
  // TeamScratchBytes of local memory. A launch with TeamRange::BackendDefault runs
  // DefaultThreadsPerTeam work items.
  inline constexpr int DefaultThreadsPerTeam = SyclLocalSize;
  inline constexpr int TeamScratchBytes      = 16384;

  void* getSYCLQueue(int number);
  INLINE_METHOD int getTeamScratchWords(int requested);
} // namespace tarch::accelerator

#endif
