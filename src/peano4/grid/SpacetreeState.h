// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace peano4::grid {
  enum class SpacetreeState {
    /**
     * Not yet a new root. Just got created, so we have to run through the
     * cloned data once, just to get it into the right order, and then we
     * can really mirror the master's traversal and send out stuff (in state
     * NewRoot).
     */
    EmptyRun,
    NewRoot,
    /**
     * Set if this tree results from a split and if this is the first
     * grid sweep when the former owner actually is in the mode
     * splitting.
     */
    NewFromSplit,
    Running,
    /**
     * Join has been triggered for this tree. Nothing is happening yet. It is
     * only the worker that updates all adjacency lists. These updates
     * however are not yet given to the master.
     */
    JoinTriggered,
    Joining,
    Joined
  };

  std::string toString(SpacetreeState state);
} // namespace peano4::grid
