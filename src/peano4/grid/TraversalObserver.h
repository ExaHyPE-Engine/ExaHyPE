// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <set>
#include <vector>

#include "peano4/parallel/parallel.h"
#include "peano4/utils/Globals.h"
#include "tarch/la/Vector.h"

namespace peano4 {
  namespace grid {
    class TraversalObserver;
    class GridControlEvent;
    struct GridTraversalEvent;
  } // namespace grid
  namespace datamanagement {
    class CellMarker;
    class FaceMarker;
    class VertexMarker;
  } // namespace datamanagement
} // namespace peano4

/**
 *
 * \section  Copy behaviour
 *
 * There is one observer by grid traversal thread and per rank. The observers
 * are generated from the original observer via the clone() operator.
 * Therefore, you never should be required to write a copy constructor. If you
 * run without multithreading but with MPI, you still have to use a
 * SpacetreeSet. The code therefore continues to copy observers.
 */
class peano4::grid::TraversalObserver {
public:
  virtual ~TraversalObserver() {}

  static constexpr int NoRebalancing = -1;

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
   * Implies that the data will then be local or had been local.
   */
  static constexpr int CreateOrDestroyPersistentGridEntity = -2;

  /**
   * Implies that the data will then be local or had been local.
   */
  static constexpr int CreateOrDestroyHangingGridEntity = -3;

  virtual void loadCell(const GridTraversalEvent& event) = 0;

  /**
   * Event is invoked per cell. It is however not called for the root cell,
   * i.e. for the cell with level 0 that does not have a parent.
   */
  virtual void enterCell(const GridTraversalEvent& event) = 0;

  virtual void leaveCell(const GridTraversalEvent& event) = 0;

  virtual void storeCell(const GridTraversalEvent& event) = 0;

  /**
   * I use the clone to create one observer object per traversal thread. So
   * between different spacetrees of one spacetree set, there can be no race
   * condition. Yet, the clone() itself could be called in parallel.
   *
   * \section  Global per-sweep actions
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
  virtual TraversalObserver* clone(int spacetreeId) = 0;

  /**
   * The tree traversal invokes this operation before beginIteration.
   *
   * \section Content
*
 * Dynamic AMR is controlled via a sequence of grid control events. Each
 * event spans a certain region and prescribes an h resolution over this
 * region. Depending on the type of the event (erase or refine), the grid
 * adopts. A simple snippet just creating a refined area in a square is
 *
 * <pre>
std::vector< peano4::grid::GridControlEvent > applications4::grid::MyObserver::getGridControlEvents() {
std::vector< peano4::grid::GridControlEvent >  controlEvents;
peano4::grid::GridControlEvent newEvent;
newEvent.setRefinementControl( peano4::grid::GridControlEvent::RefinementControl::Refine );
newEvent.setOffset( {0.0,0.0} );
newEvent.setWidth( {0.5,0.5} );
newEvent.setH( {0.05,0.05} );
controlEvents.push_back(newEvent);
return controlEvents;
}
   </pre>
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
  virtual void beginTraversal(
    const tarch::la::Vector<Dimensions, double>& x, const tarch::la::Vector<Dimensions, double>& h
  ) = 0;

  /**
   * @see beginTraversal()
   */
  virtual void endTraversal(
    const tarch::la::Vector<Dimensions, double>& x, const tarch::la::Vector<Dimensions, double>& h
  ) = 0;

  /**
   * Send local data from top level of local mesh to master and receive its
   * top-down information in return.
   *
   * The SpacetreeSet class provides some generic routines for this that you
   * can use. Simply invoke them for every data container that you use. If
   * you trigger non-blocking MPI, you don't have to wait until they are
   * finished. You can expect the calling routine that it calls
   * finishAllOutstandingSendsAndReceives() later on.
   */
  virtual void exchangeAllVerticalDataExchangeStacks(int masterId){};

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
  virtual void exchangeAllHorizontalDataExchangeStacks(bool symmetricDataCardinality){};

  /**
   * Exchange all periodic boundary data. Periodic boundary values are always
   * handled by tree 0, i.e. there's no need to distinguish ranks here. On
   * all trees that are not rank 0, this operation should immediately return.
   */
  virtual void exchangeAllPeriodicBoundaryDataStacks(){};

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
  virtual void streamDataFromSplittingTreeToNewTree(int newWorker){};
  virtual void streamDataFromJoiningTreeToMasterTree(int masterId){};

  /**
   * Wrap up all sends and receives, i.e. invoke wait() on the MPI requests.
   * The SpacetreeSet provides a generic routine for this that you can call
   * per data container in use.
   */
  virtual void finishAllOutstandingSendsAndReceives(){};

  /**
   * There are three different scenarios when we merge data:
   *
   * - We exchange data as we are at a proper parallel boundary. As we work
   *   with a non-overlapping domain decomposition and handle the individual
   *   levels separately, this can only happen for vertices and faces. It
   *   never happens for cells.
   * - We exchange data as we are at a periodic boundary. Again, this happens
   *   only for vertices and faces.
   * - We exchange data between different levels of the tree, where the
   *   parent cell is hosted on another tree than the child. In this case,
   *   we also exchange cell data.
   *
   * There are two more modes: join and fork. It is important to note that
   * these differ from the aforementioned modi, as forks and joins are
   * realised as copies. We do not merge existing data structures, but we
   * copy them. As such, the context fork and the context join do arise when
   * we discuss data exchange. They do never arise when we speak about data
   * merges. If you have a case distinction within a merge, those two modi
   * should be left out (or equipped with assertion). They should be never
   * entered.
   */
  enum class SendReceiveContext {
    BoundaryExchange,
    MultiscaleExchange,
    ForkDomain,
    JoinDomain,
    PeriodicBoundaryDataSwap
  };

  virtual void sendVertex(int position, int toStack, SendReceiveContext context, const GridTraversalEvent& event){};
  virtual void sendFace(int position, int toStack, SendReceiveContext context, const GridTraversalEvent& event){};
  virtual void sendCell(int toStack, SendReceiveContext context, const GridTraversalEvent& event){};

  virtual void receiveAndMergeVertex(
    int position, int fromStack, SendReceiveContext context, const GridTraversalEvent& event
  ){};
  virtual void receiveAndMergeFace(
    int position, int fromStack, SendReceiveContext context, const GridTraversalEvent& event
  ){};
  virtual void receiveAndMergeCell(int fromStack, SendReceiveContext context, const GridTraversalEvent& event){};

  virtual void deleteAllStacks(){};
};
