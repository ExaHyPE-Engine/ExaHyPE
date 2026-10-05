// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(PEANO_WITH_GPU_BACKEND_KOKKOS)

#include <Kokkos_Core.hpp>
#include <Kokkos_Macros.hpp>

namespace tarch::accelerator {
  /**
   * References the Kokkos team member of the running kernel and the team scratch of the launch. The
   * member exists for the whole kernel, so a nested call may copy this handle by value.
   */
  struct KokkosTeam final {
    using MemberType = Kokkos::TeamPolicy<Kokkos::DefaultExecutionSpace>::member_type;
    const MemberType& member;
    unsigned char*    scratch{nullptr};
  };
  using TeamHandle = KokkosTeam;

  inline constexpr int KokkosThreadsPerTeam = 256;
  inline constexpr int ThreadsPerWarp       = 32;
  // A team launch runs at most KokkosThreadsPerTeam threads per team with at most
  // TeamScratchBytes of team scratch. A launch with TeamRange::BackendDefault runs
  // DefaultThreadsPerTeam threads.
  inline constexpr int DefaultThreadsPerTeam = KokkosThreadsPerTeam;
  inline constexpr int TeamScratchBytes      = 16384;
} // namespace tarch::accelerator

#endif
