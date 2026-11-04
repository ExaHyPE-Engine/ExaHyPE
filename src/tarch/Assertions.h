// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"

#if PEANO_ASSERTIONS
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/accelerator.h"

namespace tarch {
#if defined(__INTEL_LLVM_COMPILER) or defined(__NVCOMPILER)
  // The compiler icpx crashes at -O0 when an OpenMP task body inlines a [[noreturn]] assertion.
  // The compiler nvc++ 26.9 at -O2 reads a wrong array entry when a loop body inlines several [[noreturn]] assertions.
  void triggerAssertion(const std::string& assertionMessage);
#else
  [[noreturn]] void triggerAssertion(const std::string& assertionMessage);
#endif
} // namespace tarch

#define TARCH_ASSERTION_STRINGIFY_IMPLEMENTATION(x) #x
#define TARCH_ASSERTION_STRINGIFY(x) TARCH_ASSERTION_STRINGIFY_IMPLEMENTATION(x)

/**
 * Terminates the program when `expr` is false and prints the file, the line, `expr` and the
 * parameters after `expr`. The device branch prints the names of these parameters only. The host
 * branch builds its message in a lambda to permit this macro in a constexpr function.
 */
// clang-format off
#define ASSERTION(expr, ...) \
  if (not static_cast<bool>(expr)) { \
    COMPILE_IF_ON_HOST( \
      tarch::triggerAssertion([&]() { \
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
        return assertionMessage.str(); \
      }()); \
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
