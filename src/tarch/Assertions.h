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
 * Define the assert macro. An assertion is given a boolean expression. If
 * the expression isn't true, the program immediately quits giving the user
 * filename and line of the assertion failed. Assertions should be used to
 * verify preconditions and invariants but may not be used to validate any
 * arguments given by users. If the assertion fails, the program quits with
 * error code -1.
 *
 * Additional values can be passed after the expression. They are printed on
 * the host if the assertion fails, e.g. assertion(x > 0, x, marker.toString()).
 *
 * GPU support
 * -----------
 * The assertion macro is split into two preprocessor branches via
 * COMPILE_IF_ON_HOST / COMPILE_IF_ON_DEVICE so that the same macro can be
 * used inside __host__ __device__ functions without dragging std::cerr
 * into device code:
 *
 *   - On host  : one log message, full parameter values, log flush, abort.
 *   - On device: backend abort with file, line, expression and parameter
 *                NAMES (values cannot be printed because operator<< is
 *                host-only).
 *
 * The device branches deliberately do not call host-only formatting,
 * tarch::instrumentation::Log::flushBeforeAssertion, or exit -- none of those are
 * available in device code.
 */
#define TARCH_ASSERTION_STRINGIFY_IMPLEMENTATION(x) #x
#define TARCH_ASSERTION_STRINGIFY(x) TARCH_ASSERTION_STRINGIFY_IMPLEMENTATION(x)

// clang-format off
#define assertion(expr, ...) \
  if (!(expr)) { \
    COMPILE_IF_ON_HOST( \
      std::ostringstream assertionMessage; \
      assertionMessage << std::scientific << std::setprecision(20) << "Assertion in file " << __FILE__ << ", line " \
                       << __LINE__ << " failed: " << #expr; \
      auto assertionAppendParameters = \
        [&](const auto&... parameters) { \
          if constexpr (sizeof...(parameters) > 0) { \
            assertionMessage << "\nParameter values:"; \
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
