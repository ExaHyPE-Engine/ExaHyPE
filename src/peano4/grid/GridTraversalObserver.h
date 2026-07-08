// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "GridControlEvent.h"

#include "peano4/Globals.h"
#include "peano4/stacks/SendReceiveContext.h"
#include "tarch/la/Vector.h"

#include <set>
#include <vector>

namespace peano4::grid {
  class GridTraversalObserver;
  struct GridControlEvent;
  struct GridTraversalEvent;
  struct PeriodicStamp;
} // namespace peano4::grid

namespace peano4::datamanagement {
  struct CellMarker;
  struct FaceMarker;
  struct VertexMarker;
} // namespace peano4::datamanagement

/**
 *
 * \section Copy behaviour
 *
 * There is one observer by grid traversal thread and per rank. The observers
 * are generated from the original observer via the clone() operator.
 * Therefore, you never should be required to write a copy constructor. If you
 * run without multithreading but with MPI, you still have to use a
 * SpacetreeSet. The code therefore continues to copy observers.
 */
class peano4::grid::GridTraversalObserver {
public:
  GridTraversalObserver()                                        = default;
  virtual ~GridTraversalObserver()                               = default;
  GridTraversalObserver(const GridTraversalObserver&)            = default;
  GridTraversalObserver& operator=(const GridTraversalObserver&) = default;
  GridTraversalObserver(GridTraversalObserver&&)                 = default;
  GridTraversalObserver& operator=(GridTraversalObserver&&)      = default;

  /**
   * Can this grid entity hold data
   *
   * I use this one to indicate that no data is associated with a grid
   * entity, as the grid entity is outside of the local computational
   * domain. The term refers explicitly to the domain decomposition,
   * i.e. this value is used to flag grid entities which are there in
   * the tree, as we always have to work with full trees, but that
   * cannot really hold any data as the user never sees them.
   */
  static constexpr int NoData = -1;

  /**
   * Create/destroy marker used by the traversal event.
   *
   * Semantics:
   * - On enter events: value indicates that the entity is newly created
   *   and therefore has no previous persistent data to pop.
   * - On leave events: value indicates that the entity is about to be
   *   destroyed and therefore must not be written persistently.
   *
   * Whether an entity is hanging is orthogonal and must be queried via the
   * corresponding marker (FaceMarker/VertexMarker).
   */
  static constexpr int CreateOrDestroyGridEntity = -2;

  enum class NodeData { Vertex, Face, Cell };

  virtual void loadNodeData(const GridTraversalEvent&) {};

  virtual void enterNode(const GridTraversalEvent&) = 0;
  virtual void leaveNode(const GridTraversalEvent&) = 0;

  virtual void storeNodeData(const GridTraversalEvent&) {};

  /**
   * I use the clone to create one observer object per traversal thread. So
   * between different spacetrees of one spacetree set, there can be no race
   * condition. Yet, the clone() itself could be called in parallel.
   *
   * \section Global per-sweep actions
   *
   * If you want to implement an operation once per sweep in a parallel
   * environment, then you can exploit the fact that the spacetree set also
   * creates an observer for the global master thread, i.e. tree no 0. So if
   * you add a statement alike
   *
   * <pre>
if (peano4::parallel::Node::isGlobalMaster(spacetreeId)) {
...
}
   </pre>
 *
 * then you can be sure that the branch body is executed only once globally
 * per grid sweep.
 *
 *
 * The counterpart of the clone operation is the destructor.
 */
  virtual GridTraversalObserver* clone(int spacetreeId) = 0;

  /**
   * The tree traversal invokes this operation before beginIteration.
   *
   * \section Content
   *
   * Dynamic AMR is controlled via a sequence of grid control events. Each
   * event spans a certain region and prescribes an h resolution over this
   * region. Depending on the type of the event (erase or refine), the grid
   * adopts.
   *
   * The entries are logically ordered. The later the entry, the more
   * important it is. So entry 2 overrules entry 1.
   */
  virtual std::vector<GridControlEvent> getGridControlEvents() const = 0;

  /**
   * Begin the traversal
   *
   * This routine is called per spacetree instance, i.e. per subtree (thread)
   * per rank. Within the usual implementation, everything will reside on the
   * call stack anyway. If the routine is called on tree no 0, this operation
   * has to establish the master data of the global root tree, i.e. ensure
   * that the data of level -1 is technically there for the subsequent
   * enterCell event, though this data is ill-defined.
   *
   * @param x Root cell coordinates
   * @param h Root cell size
   */
  virtual void
  beginTraversal(const tarch::la::Vector<DIMENSIONS, double>& x, const tarch::la::Vector<DIMENSIONS, double>& h) = 0;

  /**
   * @see beginTraversal()
   */
  virtual void
  endTraversal(const tarch::la::Vector<DIMENSIONS, double>& x, const tarch::la::Vector<DIMENSIONS, double>& h) = 0;

  virtual void setLocalTrees() {};

  /**
   * Exchange all the data along the domain boundaries. If the bool is set,
   * we do send out exactly as many elements per face or vertex as we
   * expect to receive. Therefore, the boundary exchange can optimise the
   * data exchange.
   *
   * The SpacetreeSet class provides some generic routines for this that you
   * can use. Simply invoke them for every data container that you use. If
   * you trigger non-blocking MPI, you don't have to wait until they are
   * finished. You can expect the calling routine that it calls
   * finishAllOutstandingSendsAndReceives() later on.
   */
  virtual void exchangeAllHorizontalDataExchangeStacks() {};

  /**
   * Purge this tree's periodic input stacks of entries no reader consumed
   * last sweep. Runs per tree, after the tree's traversal and before any
   * tree's periodic routing. See
   * SpacetreeSet::purgePeriodicBoundaryDataStacks.
   */
  virtual void purgePeriodicBoundaryDataStacks() {};

  /**
   * Route this tree's periodic boundary data to the trees holding the
   * wrapped partner entities: same-rank targets receive their copy directly
   * (shared memory), cross-rank targets are staged for
   * exchangeCrossRankPeriodicData(). Runs per tree, after every tree's
   * purge. See SpacetreeSet::exchangeAllPeriodicBoundaryDataStacks.
   */
  virtual void exchangeAllPeriodicBoundaryDataStacks() {};

  /**
   * Non-blocking, point-to-point exchange of the periodic boundary data whose
   * partner tree is on another rank. Called once per rank and sweep after the
   * same-rank periodic pass has staged the cross-rank entries. See
   * SpacetreeSet::exchangeCrossRankPeriodicData.
   */
  virtual void exchangeCrossRankPeriodicData() {};

  /**
   * Stream data from current tree on which this routine is called to
   * the new worker.
   *
   * @todo Not clear how this works on the worker side.
   *
   * The SpacetreeSet class provides some generic routines for this that you
   * can use. Simply invoke them for every data container that you use. If
   * you trigger non-blocking MPI, you don't have to wait until they are
   * finished. You can expect the calling routine that it calls
   * finishAllOutstandingSendsAndReceives() later on.
   */
  virtual void streamDataFromSplittingTreeToNewTree(int newWorker) {};
  virtual void streamDataFromJoiningTreeToMasterTree(int masterId) {};

  /**
   * Wrap up all sends and receives, i.e. invoke wait() on the MPI requests.
   * The SpacetreeSet provides a generic routine for this that you can call
   * per data container in use.
   */
  virtual void finishAllOutstandingSendsAndReceives() {};

  /**
   * Send/receive one routed node-data entry. For cells, position is ignored.
   * sendNodeData() reads live call-stack data before storeNodeData() and must
   * enqueue exchange data without consuming the source object. receiveNodeData()
   * receives the payload and immediately merges it into the live call-stack
   * data. For periodic boundary exchange, periodicStamp carries the entity's
   * integer identity: the send pushes it in lockstep with the payload, and the
   * receive pops the entry matching the wrapped partner of the stamp.
   */
  virtual void sendNodeData(
    NodeData                   nodeData,
    int                        position,
    int                        toStack,
    stacks::SendReceiveContext context,
    const GridTraversalEvent&  event,
    const PeriodicStamp*       periodicStamp = nullptr
  ) {};
  virtual void receiveNodeData(
    NodeData                   nodeData,
    int                        position,
    int                        fromStack,
    stacks::SendReceiveContext context,
    const GridTraversalEvent&  event,
    const PeriodicStamp*       periodicStamp = nullptr
  ) {};
};
