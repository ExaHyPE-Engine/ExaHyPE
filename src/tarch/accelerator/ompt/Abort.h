// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(PEANO_WITH_GPU_BACKEND_OMPT)

#if defined(__SPIR__)
extern "C" void __assert_fail(const char* assertion, const char* file, unsigned int line, const char* function) noexcept
  __attribute__((noreturn));
#endif

namespace tarch::accelerator {
  TARGET_DECLARE
  inline GPU_CALLABLE_METHOD void abort(const char* message) {
#if defined(__SPIR__)
    // The Intel compiler rejects printf("%s", message) in device code. Its device printf prints only string literals.
    COMPILE_IF_ON_DEVICE(__assert_fail(message, __FILE__, __LINE__, __func__);)
#else
    COMPILE_IF_ON_DEVICE(printf("%s", message); __builtin_trap();)
#endif
    COMPILE_IF_ON_HOST(std::fputs(message, stderr); std::fflush(stderr); std::_Exit(EXIT_FAILURE);)
  }
  TARGET_DECLARE_END
} // namespace tarch::accelerator

#endif
