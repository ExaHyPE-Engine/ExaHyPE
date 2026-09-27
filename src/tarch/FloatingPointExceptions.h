// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(PEANO_WITH_FPE) and defined(_MSC_VER)
#pragma float_control(precise, on)
#elif defined(PEANO_WITH_FPE) and (defined(__clang__) or defined(__INTEL_COMPILER) or defined(__INTEL_LLVM_COMPILER))
// GCC ignores this pragma in C++ with a warning, so only the compilers that accept it see it.
#pragma STDC FENV_ACCESS ON
#endif

namespace tarch {
  struct FloatingPointExceptions;
} // namespace tarch

struct tarch::FloatingPointExceptions final {
  enum Exception : unsigned int {
    None           = 0U,
    DivisionByZero = 1U << 0,
    Invalid        = 1U << 1,
    Overflow       = 1U << 2,
    Underflow      = 1U << 3,
    Inexact        = 1U << 4
  };

  using ExceptionMask = unsigned int;

  static constexpr ExceptionMask AllExceptions = DivisionByZero | Invalid | Overflow | Underflow | Inexact;

  /** Leaves underflow out, because a subnormal result is representable. */
  static constexpr ExceptionMask DefaultExceptions = DivisionByZero | Invalid | Overflow;

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
