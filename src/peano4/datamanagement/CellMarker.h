// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _PEANO4_DATAMANAGEMENT_CELL_MARKER_H_
#define _PEANO4_DATAMANAGEMENT_CELL_MARKER_H_


#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"
#include "peano4/grid/GridTraversalEvent.h"


#ifdef Parallel
  #include <mpi.h>
  #include <functional>
#endif



namespace peano4 {
  namespace datamanagement {
    struct CellMarker;
  }
}


std::ostream& operator<<( std::ostream& out, const peano4::datamanagement::CellMarker& marker );


/**
 *
 */
struct peano4::datamanagement::CellMarker {
  private:
    tarch::la::Vector<Dimensions,double>  _centre;

    tarch::la::Vector<Dimensions,double>  _h;

    bool _hasBeenRefined;
    bool _willBeRefined;
    bool _isLocal;
    bool _areAllVerticesRefined;
    bool _isOneVertexHanging;
    bool _isOneVertexCreatedOrDestroyed;

    bool _parentIsFlaggedAsChanging;

    /**
     * This flag is used to identify enclave cells.
     */
    bool _areAllVerticesInsideDomain;
    bool _invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing;

    /**
     * Entries from (0,1,2). (0,0) or (0,0,0) is the left, bottom cell.
     */
    tarch::la::Vector<Dimensions,int>  _relativePositionOfCellWithinFatherCell;
  public:
    CellMarker(const peano4::grid::GridTraversalEvent& event);

    /**
     * Has the cell been refined when we kicked off this mesh traversal?
     * This implies that the cell is refined in this sweep, as cells do
     * not change their state throughout the traversal.
     *
     * @see willBeRefined()
     */
    bool hasBeenRefined() const;

    /**
     * Will the cell be refined in the subsequent iteration? If hasBeenRefined()
     * returns false but willBeRefined() holds, we have a cell which Peano 4
     * will refine between the current and the next grid sweep. If it is the
     * other way round, Peano 4 coarsens.
     */
    bool willBeRefined() const;

    /**
     * @return x coordinate of a cell is its centre.
     */
    tarch::la::Vector<Dimensions,double>  x() const;

    /**
     * Check whether the point x is contained within the marker.
     */
    bool isContained( const tarch::la::Vector<Dimensions,double>& x ) const;
    bool overlaps( const tarch::la::Vector<Dimensions,double>& offset, const tarch::la::Vector<Dimensions,double>& size ) const;

    /**
     * @return Size of cell
     */
    tarch::la::Vector<Dimensions,double>  h() const;

    /**
     * @return Offset of cell, i.e. the bottom left vertex's coordinate
     */
    tarch::la::Vector<Dimensions,double>  getOffset() const;

    std::string toString() const;

    bool isLocal() const;

    /**
     * A enclave cell in the definition of Charrier, Hazelwood, Weinzierl is a
     * cell that is not a skeleton cell. A skeleton cell is cell which either
     *
     * - is adjacent to a resolution transitions; or
     * - is adjacent to a domain boundary.
     *
     * Enclave cells are cells that you can potentially (if your algorithm
     * allows) deploy to a task running in the background. The only thing you
     * have to do is to ensure that it is passed all data via firstprivate copy
     * and that you never refine an enclave cell.
     *
     * <h2> Dynamic load balancing </h2>
     *
     * In line with the above definition, we may label a cell as enclave cell
     * if some dynamic load balancing is triggered or actually running. In
     * this case, any enclave cell might be subject of a domain transfer. If
     * we deployed its computation to the background, we could move around a
     * cell whose computations is done by a thread currently.
     */
    bool isEnclaveCell() const;

    /**
     * A skeleton cell is a not-enclave cell
     */
    bool isSkeletonCell() const;

    tarch::la::Vector<Dimensions,int>  getRelativePositionWithinFatherCell() const;

    #if PeanoDebug>0
    /**
     * Used for debuggin
     */
    void setRelativePositionWithinFatherCell( int axis, int value );
    #endif

    #ifdef Parallel
    /**
     * To be called prior to any MPI usage of this class.
     */
    static void initDatatype();
    static void shutdownDatatype();

    /**
     * In DaStGen (the first version), I had a non-static version of the send
     * as well as the receive. However, this did not work with newer C++11
     * versions, as a member function using this as pointer usually doesn't
     * see the vtable while the init sees the object from outside, i.e.
     * including a vtable. So this routine now is basically an alias for a
     * blocking MPI_Send.
     */
    static void send(const CellMarker& buffer, int destination, int tag, MPI_Comm communicator );
    static void receive(CellMarker& buffer, int source, int tag, MPI_Comm communicator );

    /**
     * Alternative to the other send() where I trigger a non-blocking send an
     * then invoke the functor until the corresponding MPI_Test tells me that
     * the message went through. In systems with heavy MPI usage, this can
     * help to avoid deadlocks.
     */
    static void send(const CellMarker& buffer, int destination, int tag, std::function<void()> waitFunctor, MPI_Comm communicator );
    static void receive(CellMarker& buffer, int source, int tag, std::function<void()> waitFunctor, MPI_Comm communicator );

    static void sendAndPollDanglingMessages(const CellMarker& message, int destination, int tag, MPI_Comm communicator  );
    static void receiveAndPollDanglingMessages(CellMarker& message, int source, int tag, MPI_Comm communicator  );


    static MPI_Datatype  Datatype;
    #endif
};


#endif
