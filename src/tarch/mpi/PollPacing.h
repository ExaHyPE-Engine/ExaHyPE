// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"

namespace tarch::mpi {
  class PollPacing;
} // namespace tarch::mpi

/**
 * Paces the timeout bookkeeping of a polling wait. The clock read and the walk over the
 * pending-request table run on a stride that doubles while the wait lasts. A caller tests its
 * request on every iteration and paces the bookkeeping only.
 */
class tarch::mpi::PollPacing final {
public:
  INLINE_METHOD bool reportProgressNow();

private:
  static constexpr int MaxStride = 1024;

  int _stride{1};
  int _iterationsUntilNextTurn{0};
};

#include "tarch/mpi/PollPacing.cpph"
