// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "NonCriticalAssertions.h"

#include "tarch/instrumentation/Debug.h"
#include "tarch/instrumentation/Error.h"
#include "tarch/mpi/Rank.h"

namespace {
  std::atomic<int> rankWhichHasSetNonCriticalAssertion{-1};
} // namespace

/**
 * Records that this rank winds the run down. The first violation wins the exchange and writes the
 * error, every later one writes a debug message.
 */
void tarch::triggerNonCriticalAssertion(
  const std::string& file,
  const int          line,
  const std::string& expression,
  const std::string& parameterValuePairs
) {
  int expected = -1;
  if (not rankWhichHasSetNonCriticalAssertion
            .compare_exchange_strong(expected, tarch::mpi::Rank::getInstance().getRank())) {
    logDebug("A non-critical assertion failed in (" << file << ":" << line << "): " << expression);
    logDebug("A non-critical assertion has already put this rank into shutdown mode.");
    return;
  }

  logError("A non-critical assertion failed in (" << file << ":" << line << "): " << expression);
  logError(parameterValuePairs);
  logError("The application will dump the solution and shut down.");
}

bool tarch::hasNonCriticalAssertionBeenViolated() { return rankWhichHasSetNonCriticalAssertion.load() >= 0; }

/**
 * Returns the rank that raised a non-critical assertion, or -1 if no rank raised one.
 */
int tarch::getNonCriticalAssertionRank() { return rankWhichHasSetNonCriticalAssertion.load(); }

/**
 * Adopts a non-critical assertion that another rank raised. A rank that has raised one of its own
 * keeps its own value, and passing -1 leaves the value untouched.
 */
void tarch::adoptNonCriticalAssertionRank(const int rank) {
  int expected = -1;
  rankWhichHasSetNonCriticalAssertion.compare_exchange_strong(expected, rank);
}
