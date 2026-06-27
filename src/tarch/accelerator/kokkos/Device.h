// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(PEANO_WITH_GPU_BACKEND_KOKKOS)

#include <Kokkos_Core.hpp> // to define the team
#include <Kokkos_Macros.hpp>

namespace tarch::accelerator {
  // Opaque team handle wrapping a Kokkos team member. The reference points to
  // the team member parameter of the enclosing KOKKOS_LAMBDA — its lifetime
  // covers the entire outer-lambda body, so any nested call that captures
  // KokkosTeam by value remains valid.
  struct KokkosTeam {
    using member_type = Kokkos::TeamPolicy<Kokkos::DefaultExecutionSpace>::member_type;
    const member_type& m;
  };
  using TeamHandle = KokkosTeam;
} // namespace tarch::accelerator

#endif
