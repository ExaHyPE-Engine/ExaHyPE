// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "NonCriticalAssertions.h"

#include "tarch/instrumentation/Log.h"
#include "tarch/mpi/Rank.h"

namespace {
  std::atomic<int> rankWhichHasSetNonCriticalAssertion{-1};
} // namespace

/**
 * Record that this rank has to wind the run down
 *
 * The first violation wins the exchange and reports itself. Later ones only trace,
 * because every rank keeps computing with the invalid data until the final dump and
 * would otherwise repeat the same message for every patch it touches.
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
    logDebug("Non-critical assertion " << expression << " failed in (" << file << ":" << line << ")");
    return;
  }

  logError("Non-critical assertion " << expression << " failed in (" << file << ":" << line << ")");
  logError(parameterValuePairs);
  logError("Mark application to dump solution and shut down");
}

bool tarch::hasNonCriticalAssertionBeenViolated() { return rankWhichHasSetNonCriticalAssertion.load() >= 0; }

/**
 * Rank that raised a non-critical assertion, or -1 if none has.
 */
int tarch::getNonCriticalAssertionRank() { return rankWhichHasSetNonCriticalAssertion.load(); }

/**
 * Adopt a non-critical assertion that another rank raised
 *
 * A caller that reduces the rank over all ranks feeds the result back here, so every rank
 * winds down on the same grid sweep. The first violation wins, so a rank that has raised
 * one of its own keeps reporting itself. A reduction over ranks that raised nothing hands
 * back the unset value, and exchanging that for itself needs no guard of its own.
 */
void tarch::adoptNonCriticalAssertionRank(const int rank) {
  int expected = -1;
  rankWhichHasSetNonCriticalAssertion.compare_exchange_strong(expected, rank);
}
