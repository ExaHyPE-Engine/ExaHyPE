// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(PEANO_WITH_GPU_BACKEND_KOKKOS)

#include <Kokkos_Core.hpp>
#include <Kokkos_Macros.hpp>

namespace tarch::accelerator {
  /**
   * Holds the Kokkos team member of the running kernel. The member exists for the whole kernel, so
   * a nested call may copy this handle by value.
   */
  struct KokkosTeam final {
    using MemberType = Kokkos::TeamPolicy<Kokkos::DefaultExecutionSpace>::member_type;
    const MemberType& member;
  };
  using TeamHandle = KokkosTeam;
} // namespace tarch::accelerator

#endif
