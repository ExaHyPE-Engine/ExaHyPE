// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(PEANO_WITH_GPU_BACKEND_SYCL)

namespace tarch::accelerator {
  inline GPU_CALLABLE_METHOD void abort(const char* message) {
#if defined(__SPIR__)
    COMPILE_IF_ON_DEVICE(__assert_fail(message, __FILE__, __LINE__, __func__);)
#else
    COMPILE_IF_ON_DEVICE(__builtin_trap();)
#endif
    COMPILE_IF_ON_HOST(std::fputs(message, stderr); std::fflush(stderr); std::_Exit(EXIT_FAILURE);)
  }
} // namespace tarch::accelerator

#endif
