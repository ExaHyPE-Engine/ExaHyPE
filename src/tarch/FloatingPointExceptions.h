// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"

#if defined(PEANO_WITH_FPE) and defined(_MSC_VER)
#pragma float_control(precise, on)
#elif defined(PEANO_WITH_FPE) and (defined(__clang__) or defined(__INTEL_COMPILER) or defined(__INTEL_LLVM_COMPILER))
// GCC warns and ignores this pragma in C++, so only enable it on compilers that accept it.
#pragma STDC FENV_ACCESS ON
#endif

namespace tarch {
  struct FloatingPointExceptions;
} // namespace tarch

struct tarch::FloatingPointExceptions final {
  enum Exception : unsigned int {
    None           = 0u,
    DivisionByZero = 1u << 0,
    Invalid        = 1u << 1,
    Overflow       = 1u << 2,
    Underflow      = 1u << 3,
    Inexact        = 1u << 4,
  };

  using ExceptionMask = unsigned int;

  static constexpr ExceptionMask AllExceptions = DivisionByZero | Invalid | Overflow | Underflow | Inexact;

  /**
   * Selects division by zero, invalid operations, and overflow by default. Underflow remains
   * opt-in because subnormal results are representable.
   */
  static constexpr ExceptionMask DefaultExceptions = DivisionByZero | Invalid | Overflow;

  static bool areExceptionTrapsSupported() noexcept;
  static bool enableExceptionTraps(const ExceptionMask exceptions = DefaultExceptions) noexcept;
  static void disableExceptionTraps() noexcept;

  class ScopedTrap final {
  public:
    ScopedTrap(const char* const file, const int line, const ExceptionMask exceptions = DefaultExceptions);
    ~ScopedTrap();

    ScopedTrap(const ScopedTrap& other)            = delete;
    ScopedTrap& operator=(const ScopedTrap& other) = delete;
    ScopedTrap(ScopedTrap&& other)                 = delete;
    ScopedTrap& operator=(ScopedTrap&& other)      = delete;

  private:
    // A build without trap support keeps the members but never reads them.
    [[maybe_unused]] const ExceptionMask _previousExceptions;
    [[maybe_unused]] std::fenv_t         _previousEnvironment;
  };
};
