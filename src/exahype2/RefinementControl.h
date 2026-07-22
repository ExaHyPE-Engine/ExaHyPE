// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/grid/GridControlEvent.h"

namespace exahype2 {
  enum class RefinementCommand { Keep, Refine, Erase };

  /**
   * The default is coarsen as this is the lowest priority command.
   */
  RefinementCommand getDefaultRefinementCommand();

  class RefinementControl;
  class RefinementControlService;
} // namespace exahype2

/**
 * Manage refine/erase requests within ExaHyPE 2
 *
 * One of these containers lives in each RefinementCriterion action set. It collects the
 * commands a traversal raises, one grid-control event per command, and
 * RefinementControlService::merge() reduces the per-tree containers into the rank's request
 * set at the end of the traversal.
 *
 * ## Refinement process
 *
 * A criterion traversal collects the requests. The service then commits them, and Peano
 * spends two further traversals on them: the first triggers the topology change on the
 * affected vertices, the second realises it. Re-delivering an event for that second
 * traversal is the service's job, so a command carries no lifetime.
 *
 * @see RefinementControlService::finishStep()
 */
class exahype2::RefinementControl final {
public:
  friend class RefinementControlService;

  /**
   * Create a container for the requests of one solver.
   */
  explicit RefinementControl(int solverNumber = 0);

  /**
   * Drop the events collected so far.
   */
  void clear();

  /**
   * Add a refine, keep or erase request for one cell.
   */
  void addCommand(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    exahype2::RefinementCommand                  command
  );

  std::string toString() const;

private:
  struct NewEvent {
    enum class RealisationPhase { TriggerTopologyChange, RealiseTopologyChange };

    peano4::grid::GridControlEvent event;
    RealisationPhase               phase;
    int                            solverNumber;
  };

  typedef std::list<NewEvent> NewEvents;

  void addCommand(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    exahype2::RefinementCommand                  command,
    NewEvent::RealisationPhase                   phase
  );

  /**
   * Container to accumulate new events. This is a list as we may assume
   * that a lot of inserts are done per iteration.
   */
  NewEvents _newEvents;

  int _solverNumber;
};
