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
 * The clock read and the walk over the pending-request table cost more than the request test itself,
 * so they run on a stride that doubles while the wait lasts, and a saturated stride also offers the
 * core to another thread. This must never pace a call that makes progress for somebody else: A
 * partner can wait for this rank to answer before it completes the operation this loop waits for, so
 * the request test itself belongs on every iteration. Every loop holds its own instance, so the
 * stride starts tight and a short wait keeps its latency.
 */
class tarch::mpi::PollPacing final {
public:
  INLINE_METHOD bool reportProgressNow();

private:
  /**
   * Upper bound on the number of iterations between two turns.
   */
  static constexpr int MaxStride = 1024;

  int _stride{1};
  int _iterationsUntilNextTurn{0};
};

#include "tarch/mpi/PollPacing.cpph"
