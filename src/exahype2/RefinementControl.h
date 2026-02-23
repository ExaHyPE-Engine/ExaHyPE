// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/grid/GridControlEvent.h"

#include <list>
#include <vector>

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
 * If one of the flags says refine, then refine. If one of the two flags says keep and
 * noone says refine, return keep. Coarsen if and only if both say coarsen.
 */
exahype2::RefinementCommand operator&&(exahype2::RefinementCommand lhs, exahype2::RefinementCommand rhs);

std::string toString(exahype2::RefinementCommand value);

/**
 * Manage refine/erase requests within ExaHyPE 2
 *
 * This class is a very simple container which collects a point set of
 * refinement and coarsening commands. It then constructs bounding boxes for
 * each command and returns the bounding boxes to Peano 4.
 *
 * Note: Peano will internally decide to merge many many bounding boxes into
 * fewer, larger ones.
 *
 * @see peano4::grid::GridControlEvent::merge()
 *
 *
 * ## Refinement process
 *
 * We have a two stage refinement process in ExaHyPE. These are
 * ExaHyPE-specific stages and they have nothing to do with the fact that we
 * always trigger the refinement and then refine in the subsequent grid sweep,
 * i.e. it has nothing do do with the fact that ExaHyPE needs two grid sweeps
 * to realise (commit) adaptive mesh refinement:
 *
 * ExaHyPE first collects all refinement requests within this class. These
 * events are called new events. After the sweep, we roll over the refinement
 * requests. We commit them. These committed events then are delivered to the
 * grid traversal automaton. finishStep() does the roll-over, the delivery
 * happens once you call getGridControlEvents() through the AMR observer.
 *
 * Hence, the overall refinement spawns three grid traversals:
 *
 * - The container collects the refinement requests in the first sweep. Towards
 *   the end of the sweep, the requests are rolled over (committed).
 * - The second sweep grabs the refinement requests from the container and runs
 *   through the mesh. It triggers the refinement for all affected vertices.
 *   Nothing is changed yet.
 * - In the third grid sweep, the mesh is refined actually.
 *
 * Due to the overall three-step mechanism, we make the bookkeeping of the
 * events persistent. Each event can potentially survive a couple of sets
 * within the new events. It is rolled over multiple times until it "expires"
 * within the new events.
 *
 * @see _committedEvents
 *
 */
class exahype2::RefinementControl {
public:
  typedef std::list<std::pair<peano4::grid::GridControlEvent, int>> NewEvents;

  friend class RefinementControlService;

  /**
   * @param tolerance How much should the code add around each
   *   grid control. By default we use one percent additional
   *   tolerance.
   */
  RefinementControl(double tolerance = 0.01);
  virtual ~RefinementControl() = default;

  /**
   * Clears the new events
   *
   * Has no affect for the committed events. This routine should only ever be
   * called by the action sets on their local refinement control assembly.
   * The global one is persistent and should not be cleared by the user, as
   * you would loose all those events that should remain alive. Therefore,
   * there's no clear for the service.
   */
  void clear();

  /**
   * Add a new command.
   *
   * Depending on the context, we add a new refinement instruction
   * interpreting x and h. If you refine, I divide h by three.
   *
   * ## Lifetime of a refinement event
   *
   * If a refinement event is triggered, we keep it alive for a while. This
   * avoids oscillations (refinement events always overwrite coarsening) and
   * it helps ranks to catch up - a code might not be able to refine
   * immediately as it implements a Runge-Kutta scheme, e.g. In this case, we
   * keep the event alive and then realise it once the code is ready to do
   * so.
   *
   * @param x Centre of the cell for which this command is triggered. This is
   *   typically marker.getX().
   * @param h Size of this cell. Typically marker.getH().
   * @param lifetime Has to be greater equal 1.
   */
  void addCommand(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    exahype2::RefinementCommand                  command,
    int                                          lifetime
  );

  std::string toString() const;

private:
  static tarch::logging::Log _log;

  /**
   * We blow up the region around refinement criterion slightly.
   */
  const double _tolerance;

  /**
   * Container to accumulate new events. This is a list as we may assume
   * that a lot of inserts are done per iteration.
   */
  NewEvents _newEvents;
};
