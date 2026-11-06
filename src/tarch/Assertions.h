// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"

#if PEANO_ASSERTIONS
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/accelerator.h"

namespace tarch {
  // An assertion message can call the function that contains the assertion. GCC reports the
  // assertion lambda of such a function as infinite recursion if triggerAssertion() is [[noreturn]].
  /*[[noreturn]]*/ void triggerAssertion(const std::string& assertionMessage);
} // namespace tarch

#define TARCH_ASSERTION_STRINGIFY_IMPLEMENTATION(x) #x
#define TARCH_ASSERTION_STRINGIFY(x) TARCH_ASSERTION_STRINGIFY_IMPLEMENTATION(x)

#if defined(__INTEL_LLVM_COMPILER)
// The compiler icpx miscompiles or crashes on an OpenMP region at -O0 if a path of the region ends
// in __builtin_unreachable().
#define TARCH_ASSERTION_UNREACHABLE
#elif defined(__NVCOMPILER)
// The compiler nvc++ 26.9 at -O2 reads a wrong array entry in a loop body that inlines several
// assertions whose failed branch never returns.
#define TARCH_ASSERTION_UNREACHABLE
#else
// GCC reports out-of-bounds accesses on the path after a failed assertion unless the host branch
// ends with __builtin_unreachable().
#define TARCH_ASSERTION_UNREACHABLE __builtin_unreachable();
#endif

/**
 * Terminates the program when `expr` is false and prints the file, the line, `expr` and the
 * parameters after `expr`. The device branch prints the names of these parameters only. The host
 * branch builds its message in a lambda to permit this macro in a constexpr function.
 */
// clang-format off
#define ASSERTION(expr, ...) \
  if (not static_cast<bool>(expr)) { \
    COMPILE_IF_ON_HOST( \
      [&]() __attribute__((noinline)) { \
        std::ostringstream assertionMessage; \
        assertionMessage << std::scientific << std::setprecision(20) << "Assertion in file " << __FILE__ << ", line " \
                         << __LINE__ << " failed: " << #expr; \
        auto assertionAppendParameters = \
          [&](const auto&... parameters) { \
            if constexpr (sizeof...(parameters) > 0) { \
              assertionMessage << "\nParameters:"; \
              assertionMessage << "\n  " << std::left << std::setw(8) << "Index" << " | Value"; \
              assertionMessage << "\n  " << std::left << std::setw(8) << "-----" << " | -----"; \
              int  parameterNumber          = 0; \
              auto assertionAppendParameter = [&](const auto& parameter) { \
                assertionMessage << "\n  " << std::left << std::setw(8) << parameterNumber++ << " | " << parameter; \
              }; \
              (assertionAppendParameter(parameters), ...); \
            } \
          }; \
        assertionAppendParameters(__VA_ARGS__); \
        tarch::triggerAssertion(assertionMessage.str()); \
      }(); \
      TARCH_ASSERTION_UNREACHABLE \
    ) \
    COMPILE_IF_ON_DEVICE( \
      tarch::accelerator::abort( \
        "Assertion in file " __FILE__ \
        ", line " TARCH_ASSERTION_STRINGIFY(__LINE__) " failed: " #expr \
        "\n" __VA_OPT__("  Parameters: " #__VA_ARGS__ "\n") \
      ); \
    ) \
  }
// clang-format on
#else
#define ASSERTION(...)
#endif
