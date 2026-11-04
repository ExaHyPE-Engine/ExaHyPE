// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "NonCriticalAssertions.h"

#include "tarch/Assertions.h"
#include "tarch/instrumentation/Debug.h"
#include "tarch/instrumentation/Error.h"
#include "tarch/mpi/Rank.h"

namespace {
  std::atomic<int> rankWhichHasSetNonCriticalAssertion{-1};
} // namespace

/**
 * Records the rank of the first violated non-critical assertion. The first violation writes the
 * requirement and the values as an error. Every later violation writes a debug message.
 */
void
tarch::triggerNonCriticalAssertion(const std::string& requirement, const std::string& values) {
  assertion(not requirement.empty());
  int expected = -1;
  if (not rankWhichHasSetNonCriticalAssertion
            .compare_exchange_strong(expected, tarch::mpi::Rank::getInstance().getRank())) {
    logDebug(requirement);
    logDebug("The rank stops after an earlier non-critical assertion.");
    return;
  }

  logError(requirement);
  logError(values);
  logError("The rank writes its output and shuts down.");
}

bool
tarch::hasNonCriticalAssertionBeenViolated() {
  return rankWhichHasSetNonCriticalAssertion.load() >= 0;
}

/**
 * Returns the rank that raised a non-critical assertion. Without a non-critical assertion, the
 * result is -1.
 */
int
tarch::getNonCriticalAssertionRank() {
  return rankWhichHasSetNonCriticalAssertion.load();
}

/**
 * Adopts a non-critical assertion that another rank raised. A rank that has raised one of its own
 * keeps its own value. The rank -1 leaves the value untouched.
 */
void
tarch::adoptNonCriticalAssertionRank(const int rank) {
  int expected = -1;
  rankWhichHasSetNonCriticalAssertion.compare_exchange_strong(expected, rank);
}
