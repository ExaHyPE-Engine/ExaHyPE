// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Assertions.h"

#if PEANO_ASSERTIONS
#include "tarch/Termination.h"

void tarch::triggerAssertion(const std::string& assertionMessage) { tarch::terminateWithError(assertionMessage); }
#endif

namespace {
  [[maybe_unused]] void compileAssertionMacro(
    const int         value,
    const int         expected,
    const double      otherValue,
    const char* const message
  ) {
    assertion(value == expected);
    assertion(value == expected, value, expected, otherValue, message);
  }
} // namespace
