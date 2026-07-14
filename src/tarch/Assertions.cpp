// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/Assertions.h"

#if PEANO_ASSERTIONS
#include "tarch/instrumentation/Error.h"

namespace tarch {
  namespace mpi {
    void abort(int errorCode);
  } // namespace mpi

  void triggerAssertion(const std::string& assertionMessage) {
    static std::atomic<int> assertionOutputState{0};
    int                     assertionOutputExpectedState = 0;
    if (assertionOutputState.compare_exchange_strong(assertionOutputExpectedState, 1)) {
      std::cout.flush();
      std::cerr.flush();

      logError(assertionMessage);
      tarch::instrumentation::Log::flushBeforeAssertion();
      assertionOutputState.store(2);
    } else {
      while (assertionOutputState.load() == 1) {
      }
    }
    tarch::mpi::abort(-1);
  }
} // namespace tarch
#endif

namespace {
  [[maybe_unused]] void
  compileAssertionMacro(const int value, const int expected, const double otherValue, const char* const message) {
    assertion(value == expected);
    assertion(value == expected, value, expected, otherValue, message);
  }
} // namespace
