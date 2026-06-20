// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(WITH_GPU_KOKKOS)

#include "Device.h"

#include <Kokkos_Abort.hpp>

namespace tarch::accelerator {
  inline KOKKOS_FUNCTION void abort(const char* message) {
    COMPILE_IF_ON_DEVICE(
      ::Kokkos::abort(message);
    )
    COMPILE_IF_ON_HOST(
      ::Kokkos::abort(message);
    )
  }
} // namespace tarch::accelerator

#endif
