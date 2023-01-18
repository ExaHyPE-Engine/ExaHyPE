// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <vector>
#include <list>

#include "tarch/services/Service.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "peano4/grid/GridControlEvent.h"

namespace exahype2 {
  enum class RefinementCommand {
    Keep, Refine, Erase
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
 * refinement and coarsening commands. It then constructs bounding boxes for
 * each command and returns the bounding boxes to Peano 4.
 *
 * Note: Peano will internally decide to merge many many bounding boxes into
 * fewer, larger ones.
 *
 * @see peano4::grid::merge()
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
 * events persistent. Each event can potentially survive a couple of sets.
 *
 * @see _committedEvents
 *
 *
 *
 * ## Multithreading
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
class exahype2::RefinementControl: public tarch::services::Service {
  public:
    RefinementControl(double tolerance = 0.1);
    virtual ~RefinementControl();

    void receiveDanglingMessages() override;
    void shutdown() override;

    /**
     * Clears the new events
     *
     * Has no affect for the committed events. This routine should only ever be
     * called by the action sets on their local refinement control assembly.
     * The global one is persistent and should not be cleared by the user, as
     * you would loose all those events that should remain alive.
     */
    void clear();

    /**
     * @see clear()
     */
    std::vector< peano4::grid::GridControlEvent >  getGridControlEvents() const;

    /**
     * Add a new command.
     *
     * Depending on the context, we add a new refinement instruction
     * interpreting x and h. If you refine, I divide h by three.
     *
     * @param x Centre of the cell for which this command is triggered
     * @param h Size of this cell
     */
    void addCommand(
      const tarch::la::Vector<Dimensions,double>&  x,
      const tarch::la::Vector<Dimensions,double>&  h,
      exahype2::RefinementCommand                  command,
      int                                          lifetime
    );

    void merge( const RefinementControl& control );

    /**
     * Should be called after each traversal per rank.
     *
     * This routine rolls over _newEvents into the _committedEvents if they are still
     * alive.
     *
     * ## MPI handing
     *
     * The MPI data exchange here is non-trivial, as we do not know which rank has how
     * many messages. So we run in three steps: First, we send the committed events out to
     * all partners. These sends are done non-blocking. They logically are a broadcast.
     * Second, we loop over all the arising MPI_Requests and poll until they are finished.
     * So we know all data has gone out. This is a while loop with the predicate
     *
     *       not sendRequests.empty()
     *
     * Third, we hook into this while loop and probe for all the other ranks if they
     * have sent us anything. So while we wait for our sends to go out, we poll the other
     * ranks for their data, integrate this information into our data structures and
     * therefore also free any MPI queues.
     *
     * As we build up the incoming data while we wait for our sends to go out, we have to
     * send from a copy of the actual data (copyOfCommittedEvents): We cannot alter the
     * outgoing data due to polls for incoming messages while we have still pending sends.
     *
     * It is important to work with non-blocking calls here, as the code otherwise tends
     * to deadlock if we have a lot of events. This is due to MPI falling back to
     * rendeszvouz message passing if the data that is on-the-fly becomes too large.
     */
    void finishStep();

    std::string toString() const;

  private:
    static tarch::logging::Log  _log;

    static tarch::multicore::BooleanSemaphore  _semaphore;

    /**
     * I need a tag of my own to exchange control info after each step.
     */
    static int _reductionTag;

    /**
     * We blow up the region around refinement criterion slightly.
     */
    const double _Tolerance;

    typedef std::list< std::pair<peano4::grid::GridControlEvent,int> >  NewEvents;

    /**
     * Container to accumulate new events. This is a list as we may assume
     * that a lot of inserts are done per iteration.
     */
    NewEvents    _newEvents;

    /**
     * Container with all the valid events. Is an extract from _newEvents
     * which is built up in finishStep() and then handed out to Peano once
     * it asks.
     *
     * ## Event lifetime
     *
     * The individual events are not only copied over. Each event is annotated
     * with its lifetime. That is, events might remain active over multiple
     * iterations. This operation decrements the lifespan and, after that,
     * copies those events that are still alive over into the result.
     *
     * ## Optimisation
     *
     * The raw events might become really large over time. However, I decided to
     * keep this routine clear of any optimisation. It is the grid which has
     * to clean up the refinement events appropriately.
     *
     * ## MPI data excange
     *
     * I originally wanted to use an MPI reduction to have a consistent view of
     * all refinement controls. However, this seems not to work, as some ranks
     * can be out of sync. So what I do instead now is a delayed broadcast: Every
     * rank sends out its committed events to all others and appends all incoming
     * ones to its own set.
     *
     * This simple implementation also works for our dynamic event sets, where we
     * do not know a priori how many events are triggered by a rank. Still, it can
     * happen that event views are out-of-sync. This is not a problem here:
     *
     * The actual grid changes are all triggered via vertices, so we never obtain
     * an invalid grid. The individual grid events have a lifetime and thus are
     * active over multiple iterations. Hence, they will be merge at one point.
     *
     * We have to split up the send and receive loop such that we first send out
     * all stuff and then receive.
     *
     */
    std::vector< peano4::grid::GridControlEvent >  _committedEvents;

    #ifdef Parallel
    std::vector<MPI_Request*>                      _sendRequests;
    std::vector< peano4::grid::GridControlEvent >  _copyOfCommittedEvents;

    void freeAllPendingSendRequests();
    void triggerSendOfCopyOfCommittedEvents();
    #endif
};

