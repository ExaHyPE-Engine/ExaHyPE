// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"

#if PEANO_ASSERTIONS
#include "tarch/accelerator/accelerator.h"

namespace tarch {
  void triggerAssertion(const std::string& assertionMessage);
} // namespace tarch

/**
 * Quits the program with error code -1 when the expression evaluates to false and prints the file
 * name, the line, the expression and every value passed after the expression. An assertion states
 * a precondition or an invariant and never validates an argument a user supplies. The device
 * branch prints the parameter names without their values, because operator<< runs on the host
 * only.
 */
#define TARCH_ASSERTION_STRINGIFY_IMPLEMENTATION(x) #x
#define TARCH_ASSERTION_STRINGIFY(x) TARCH_ASSERTION_STRINGIFY_IMPLEMENTATION(x)

// clang-format off
#define assertion(expr, ...) \
  if (not(expr)) { \
    COMPILE_IF_ON_HOST( \
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
#define assertion(...)
#endif
