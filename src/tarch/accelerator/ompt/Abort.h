// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(WITH_GPU_OMPT)

#include <cstdio>
#include <cstdlib>

namespace tarch::accelerator {
  TARGET_DECLARE
  inline GPU_CALLABLE_METHOD void abort(const char* message) {
    COMPILE_IF_ON_DEVICE(
      printf("%s", message);
      __builtin_trap();
    )
    COMPILE_IF_ON_HOST(
      std::fputs(message, stderr);
      std::fflush(stderr);
      std::_Exit(EXIT_FAILURE);
    )
  }
  TARGET_DECLARE_END
} // namespace tarch::accelerator

#endif
