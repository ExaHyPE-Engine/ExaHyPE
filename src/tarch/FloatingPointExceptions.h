// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "CompilerSpecificSettings.h"

#if defined(WITH_FPE) && defined(_MSC_VER)
#pragma float_control(precise, on)
#elif defined(WITH_FPE) && (defined(__clang__) || defined(__INTEL_COMPILER) || defined(__INTEL_LLVM_COMPILER))
// GCC warns and ignores this pragma in C++, so only enable it on compilers that accept it.
#pragma STDC FENV_ACCESS ON
#endif

namespace tarch {
  struct FloatingPointExceptions {
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

    static constexpr ExceptionMask DefaultExceptions = DivisionByZero | Invalid | Overflow;

#if defined(WITH_FPE)
    static bool areExceptionTrapsSupported() noexcept;
    static bool enableExceptionTraps(ExceptionMask exceptions = DefaultExceptions) noexcept;
    static void disableExceptionTraps() noexcept;

    class ScopedTrap {
    public:
      ScopedTrap(const char* file, int line, ExceptionMask exceptions = DefaultExceptions);
      ~ScopedTrap();

      ScopedTrap(const ScopedTrap&)            = delete;
      ScopedTrap& operator=(const ScopedTrap&) = delete;

    private:
      const char* _file;
      int         _line;
      bool        _trapEnabled;
    };
#else
    static constexpr bool areExceptionTrapsSupported() noexcept { return false; }
    static constexpr bool enableExceptionTraps(ExceptionMask = DefaultExceptions) noexcept { return false; }
    static constexpr void disableExceptionTraps() noexcept {}

    class ScopedTrap {
    public:
      ScopedTrap(const char*, int, ExceptionMask = DefaultExceptions) noexcept {}
    };
#endif
  };
} // namespace tarch
