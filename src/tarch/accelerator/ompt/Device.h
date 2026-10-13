// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(PEANO_WITH_GPU_BACKEND_OMPT)

#include "tarch/accelerator/CompileTarget.h"

// A static constexpr member of a class template that only host code reads leaves its offloading
// entry without a device address. Such an entry stops the compilation of the translation unit or
// the loading of the device image.
namespace tarch::accelerator {
  TARGET_DECLARE
  // A team launch provides at most TeamScratchBytes of team scratch and asks for at most
  // OmptThreadsPerTeam threads per team. A launch with TeamRange::BackendDefault asks for
  // DefaultThreadsPerTeam threads.
  constexpr int TeamScratchBytes      = 16384;
  constexpr int OmptThreadsPerTeam    = 256;
  constexpr int ThreadsPerWarp        = 32;
  constexpr int DefaultThreadsPerTeam = OmptThreadsPerTeam;

  /**
   * Holds the team scratch of the running team. The threads of a team read their indices from the
   * OpenMP runtime.
   */
  struct OmptTeam final {
    unsigned char* scratch{nullptr};
  };
  using TeamHandle = OmptTeam;
  TARGET_DECLARE_END
} // namespace tarch::accelerator

#endif
