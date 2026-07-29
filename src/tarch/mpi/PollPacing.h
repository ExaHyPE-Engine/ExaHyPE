// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"

namespace tarch::mpi {
  class PollPacing;
} // namespace tarch::mpi

/**
 * Paces the timeout bookkeeping of a polling wait.
 *
 * A wait loop reads the clock and walks the pending-request table to decide whether to warn or to
 * abort. That costs more than the request test itself, and nothing outside the rank depends on it, so
 * it runs on a stride that doubles while the wait lasts. A saturated stride means the wait has become
 * long, so the thread also offers its core to another thread, which may be the one that still has to
 * send what this loop waits for.
 *
 * This must never pace a call that makes progress for somebody else. A partner can be waiting for this
 * rank to answer before it can complete the very operation this loop waits for, so the request test
 * itself belongs on every iteration.
 *
 * Every loop holds its own instance, so the stride starts tight and a short wait keeps its latency.
 */
class tarch::mpi::PollPacing final {
public:
  INLINE_METHOD bool reportProgressNow();

private:
  /**
   * Upper bound on the number of iterations between two turns. A request test costs a few hundred
   * nanoseconds, so the bookkeeping still runs thousands of times per second.
   */
  static constexpr int MaxStride = 1024;

  int _stride{1};
  int _iterationsUntilNextTurn{0};
};

#include "PollPacing.cpph"
