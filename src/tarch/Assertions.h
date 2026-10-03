// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"

#if PEANO_ASSERTIONS
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/accelerator.h"

namespace tarch {
  void triggerAssertion(const std::string& assertionMessage);
} // namespace tarch

#define TARCH_ASSERTION_STRINGIFY_IMPLEMENTATION(x) #x
#define TARCH_ASSERTION_STRINGIFY(x) TARCH_ASSERTION_STRINGIFY_IMPLEMENTATION(x)

/**
 * Terminates the program when expr is false and prints the file, the line, expr and every value
 * after expr. The device branch prints the parameter names without values, because operator<< runs
 * on the host only. The host branch builds the message in a lambda, so a constexpr function may
 * contain an assertion.
 */
// clang-format off
#define assertion(expr, ...) \
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
#define assertion(...)
#endif
