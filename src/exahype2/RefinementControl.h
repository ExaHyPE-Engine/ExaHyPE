// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_REFINEMENT_CONTROL_H_
#define _EXAHYPE2_REFINEMENT_CONTROL_H_


#include <vector>
#include <list>

#include "peano4/grid/GridControlEvent.h"

#include "tarch/multicore/BooleanSemaphore.h"

namespace exahype2 {
  enum class RefinementCommand {
    Keep, Refine, Coarsen
  };

  /**
   * The default is coarsen as this is the lowest priority command.
   */
  RefinementCommand getDefaultRefinementCommand();

  class RefinementControl;
}


/**
 * If one of the flags says refine, then refine. If one of the two flags says keep and
 * noone says refine, return keep. Coarsen if and only if both say coarsen.
 */
exahype2::RefinementCommand operator&&( exahype2::RefinementCommand lhs, exahype2::RefinementCommand rhs);

std::string toString( exahype2::RefinementCommand value );

/**
 * Manage refine/erase requests within ExaHyPE 2
 *
 * This class is a very simple container which collects a point set of
 * refinement and coarsening commands. It then constructs bounding boxes from
 * these guys and returns the bounding boxes to Peano 4.
 *
 *
 * <h2> Refinement process </h2>
 *
 * We have a two stage refinement process. This is yet another two stages and
 * has nothing to do with the fact that we always trigger the refinement and
 * then refine in the subsequent grid sweep: ExaHyPE first collects all
 * refinement requests within this class. After the sweep, we roll over the
 * refinement requests and deliver them to the grid traversal automaton.
 * finishStep() does the roll-over, the delivery happens once you call
 * getGridControlEvents().
 *
 * So the refinement happens actually in actually three steps/grid traversals:
 *
 * - The container collects the refinement requests in the first sweep. Towards
 *   the end of the sweep, the requests are rolled over.
 * - The second sweep grabs the refinement requests from the container and runs
 *   through the mesh. It triggers the refinement for all affected vertices.
 *   Nothing is changed yet.
 * - In the third grid sweep, the mesh is refined actually.
 *
 * Due to the overall three-step mechanism, we make the bookkeeping here quite
 * persistent. There is a list of new events and one we finish a traversal, we
 * move the elements in there over into a set of committed events. The latter
 * is erased only when some other class grabs it.
 *
 *
 * <h2> Multithreading </h2>
 *
 * As we use the class within a multithreaded environment, things become a
 * little bit more complicated. Peano 4 creates an observer object per
 * spacetree through a clone from observer no -1 (per rank, so this observer
 * is not really an observer. It is rather an observer prototype) and then
 * asks this observer how to refine. So we design the following pattern:
 *
 * - When an observer is created, it creates its own local refinement control.
 * - We hand out refine/erase commands from the global one as this is our
 *   globally valid repository.
 * - We accumulate the refine/erase instructions locally. So we clear() our
 *   local copy when we start to run through the grid and then pipe all
 *   commands into this local object.
 * - When we are done, we merge the local copy back.
 *
 * So far, everything is simple and straightforward. The only tricky part is
 * when to decide when to kick out old refine/coarsen commands. The prototype
 * refinement control object is kind of a repository where we take copies from,
 * but we never know how many copies will be created in the end. So we make
 * each local copy notify the global guy that it will accumulate something, i.e.
 * eventually merge() its content into the global object. As a result, the
 * global can work with two buffers - an accumulation buffer and a valid one.
 * Whenever we know that all copies have merged their stuff back, we swap/update
 * the buffers.
 *
 *
 */
class exahype2::RefinementControl {
  public:
    RefinementControl(double tolerance = 0.1);

    void clear();

    /**
     * Is not const, as it also erases the committed events. These are now
     * delivered, so there's place more.
     */
    std::vector< peano4::grid::GridControlEvent >  getGridControlEvents();

    /**
	   * @param x Centre of the cell for which this command is triggered
	   * @param h Size of this cell
	   */
    void addCommand(
      const tarch::la::Vector<Dimensions,double>&  x,
      const tarch::la::Vector<Dimensions,double>&  h,
      exahype2::RefinementCommand                  command,
      bool                                         invokedByGridConstruction
    );

    void merge( const RefinementControl& control );

    /**
     * Should be called after each traversal per rank.
     */
    void finishStep();

    std::string toString() const;
  private:
	  static tarch::logging::Log  _log;

	  static tarch::multicore::BooleanSemaphore  _semaphore;

	  /**
	   * We blow up the region around refinement criterion slightly.
	   */
	  const double _Tolerance;

	  /**
	   * Container to accumulate new events. This is a list as we may assume
	   * that a lot of inserts are done per iteration.
	   */
	  std::list< peano4::grid::GridControlEvent >    _newEvents;

	  /**
	   * Container with all the valid events. This is a vector as it changes
	   * infrequently and as the classes that request the events expect a
	   * vector.
	   */
	  std::vector< peano4::grid::GridControlEvent >  _committedEvents;
};

#endif
