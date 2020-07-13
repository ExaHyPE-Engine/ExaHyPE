// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _PEANO4_GRID_TRAVERSAL_OBSERVER_H_
#define _PEANO4_GRID_TRAVERSAL_OBSERVER_H_


#include "peano4/parallel/parallel.h"
#include "peano4/utils/Globals.h"
#include "tarch/la/Vector.h"


#include <vector>
#include <set>


namespace peano4 {
  namespace grid {
    class TraversalObserver;
    class GridControlEvent;
    class GridTraversalEvent;
  }
  namespace datamanagement {
    class CellMarker;
    class FaceMarker;
    class VertexMarker;
  }
}


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

    static constexpr int NoRebalancing                       = -1;

    /**
     * I use this one to indicate that no data is associated with a grid entity,
     * as the grid entity is outside of the domain.
     */
    static constexpr int NoData                              = -1;

	  /**
	   * Implies that the data will then be local or had been local.
	   */
    static constexpr int CreateOrDestroyPersistentGridEntity = -2;

    /**
     * Implies that the data will then be local or had been local.
     */
    static constexpr int CreateOrDestroyHangingGridEntity    = -3;

    /**
     * Event is invoked per cell. It is however not called for the root cell,
     * i.e. for the cell with level 0 that does not have a parent.
     */
    virtual void enterCell(
      const GridTraversalEvent&  event
    ) = 0;


    virtual void leaveCell(
      const GridTraversalEvent&  event
    ) = 0;

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
	   * \section  Content
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
  	virtual std::vector< GridControlEvent > getGridControlEvents() = 0;

  /**
   * We do not really need stack numbers et al here, as everything will
   * reside on the call stack anyway. If the routine is called on tree
   * no 0, this operation has to establish the master of the global root
   * tree (which usually is ill-defined, but the data should be there
   * anyway).
   */
  virtual void beginTraversal(
    const tarch::la::Vector<Dimensions,double>&  x,
    const tarch::la::Vector<Dimensions,double>&  h
  ) = 0;

  virtual void endTraversal(
    const tarch::la::Vector<Dimensions,double>&  x,
    const tarch::la::Vector<Dimensions,double>&  h
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
  virtual void exchangeAllVerticalDataExchangeStacks( int masterId ) {};

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
  virtual void exchangeAllHorizontalDataExchangeStacks( bool symmetricDataCardinality ) {};

  /**
   * Exchange all periodic boundary data. Periodic boundary values are always
   * handled by tree 0, i.e. there's no need to distinguish ranks here. On
   * all trees that are not rank 0, this operation should immediately return.
   */
  virtual void exchangeAllPeriodicBoundaryDataStacks() {};

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
  virtual void streamDataFromSplittingTreeToNewTree( int newWorker ) {};
  virtual void streamDataFromJoiningTreeToMasterTree( int masterId ) {};

  /**
   * Wrap up all sends and receives, i.e. invoke wait() on the MPI requests.
   * The SpacetreeSet provides a generic routine for this that you can call
   * per data container in use.
   */
  virtual void finishAllOutstandingSendsAndReceives() {};

  enum class SendReceiveContext {
    BoundaryExchange,
    MultiscaleExchange,
    Rebalancing
  };

  virtual void sendVertex(int inOutStack, int relativePositionOnInOutStack, int toStack, SendReceiveContext context, const peano4::datamanagement::VertexMarker& marker) {};
  virtual void sendFace(int inOutStack, int relativePositionOnInOutStack, int toStack, SendReceiveContext context, const peano4::datamanagement::FaceMarker& marker) {};
  virtual void sendCell(int inOutStack, int toStack, SendReceiveContext context, const peano4::datamanagement::CellMarker& marker) {};

  virtual void receiveAndMergeVertex(const GridTraversalEvent&  event, int positionWithinCell, int inOutStack, int relativePositionOnInOutStack, int fromStack, SendReceiveContext context, const peano4::datamanagement::VertexMarker& marker) {};
  virtual void receiveAndMergeFace(const GridTraversalEvent&  event, int positionWithinCell, int inOutStack, int relativePositionOnInOutStack, int fromStack, SendReceiveContext context, const peano4::datamanagement::FaceMarker& marker) {};

  virtual void deleteAllStacks() {};
};

#endif
