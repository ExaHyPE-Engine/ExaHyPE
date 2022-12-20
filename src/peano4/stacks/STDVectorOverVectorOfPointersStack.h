// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <vector>
#include <algorithm>


#include "stacks.h"
#include "STDVectorStack.h"


#include "peano4/parallel/Node.h"
#include "peano4/grid/GridVertex.h"


#include "tarch/logging/Log.h"
#include "tarch/Assertions.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/IntegerMessage.h"
#include "tarch/multicore/Tasks.h"

#include "peano4/grid/TraversalObserver.h"



namespace peano4 {
  namespace stacks {
    template <class T>
    class STDVectorOverVectorOfPointersStack;
  }
}


/**
 * Administer vector over pointers
 *
 * I assume that T is in itself a vector over pointers. Usually, I realise it
 * as a subclass of std::vector, i.e. by inheritance, as I need some additional
 * routines and slightly different semantics to std::vector. You could,
 * obviously, also write a whole new class providing all the functions used
 * here.
 *
 * The list of additional routines or routines with special semantics that I
 * need are
 *
 * - clone()
 * - clear()
 * - merge()
 * - void setDebugX( const tarch::la::Vector<Dimensions,double>& data );
 * - void setDebugH( const tarch::la::Vector<Dimensions,double>& data );
 * - tarch::la::Vector<Dimensions,double> getDebugX() const;
 * - tarch::la::Vector<Dimensions,double> getDebugH() const;
 *
 * as well as the static routines
 *
 * - static bool send(const peano4::datamanagement::VertexMarker& marker);
 * - static bool receiveAndMerge(const peano4::datamanagement::VertexMarker& marker);
 * - static bool storePersistently(const peano4::datamanagement::VertexMarker& marker);
 * - static bool loadPersistently(const peano4::datamanagement::VertexMarker& marker);
 *
 * The ...Debug... routines are not required in release mode.
 * Finally, the type T has to provide a typedef DoFType which says to which the
 * underlying pointer points to.
 *
 * ## Data ownership
 *
 * The vectors don't actually own any data for this particular stack type.
 * The data resides on the heap and the vectors are mere meta data structures
 * referencing the core data. It is consequently (usually) not copied. We can
 * however happily copy around the meta data on one core or clear some replica or
 * do whatever we want - the actual data stays at the same location on the heap.
 *
 * The stack class works with shallow copies/data ownership.
 *
 * If you clone() a stack, there's no need to clone non-local particles. So the
 * clone is shallow, but it is typically not trivial. You can amend the clone()
 * in your set implementation, as the vector wrapper itself simply calls your
 * container's clone().
 *
 *
 * ## Shared memory parallelisation
 *
 * In principle, shared memory parallelisation would work out of the box. You
 * could use the std::vector copy constructor which automatically copies the
 * pointers to the particles, i.e. all particles reside in the global heap, and
 * multiple subtrees then host pointers within their stacks to these heap
 * objects.
 *
 * This is not the strategy I recommend within Peano, as it does not mirror the
 * design decisions of the normal stacks, where I work with strictly separated
 * (non-overlapping) data domains, and synchronise all data after each grid
 * sweep. It also naturally makes the code vulnerable to race conditions. Yet,
 * you can still work with such a shallow data model.
 *
 * Before we discuss different realisation variants, we recognise that a vertex
 * at a domain boundary pipes its data onto different stacks: the local stack and
 * one stack per communication partner. These communication partner stacks hold
 * shallow copies. So far, nothing is copied really from Peano's point of view.
 *
 * We further more recommend that user codes delete all remote data before they
 * trigger any communication: They should simply throw away what they've got
 * from somewhere else. Particles, e.g., reside on the tree that holds the cell
 * in which the particle is contained. The particle is sent out per grid sweep
 * to all neighbours. So they can throw away stuff - an updated version will be
 * sent in later anyway.
 *
 *
 * ### Shallow copies, i.e. shared objects
 *
 * When you work with shallow copies, all stack entries are literally pointers
 * to the heap, and multiple trees per rank all point to the same heap objects.
 * It is the user code responsibility to ensure that no data races arise when
 * we access the heap objects.
 *
 * To realise this behaviour, let clone() simply copy over the pointers (so act
 * the same way as the std::vector copy constructor), and delegate clear() to
 * the standard std::vector::clear(). That means, the clear erases the pointers,
 * but not the objects the pointers are pointing to.
 *
 * ### Deep copies
 *
 * The clear() operation remains a shallow clear, i.e. it only eliminates the
 * pointers but does not delete any actual data. We expect the actual grid
 * traversal to throw away remote content after the grid sweep. See the
 * comments above.
 *
 * clone() is, in principle, also relatively simple: Ensure that _debugX and
 * _debugH are cloned, i.e. copied over. Otherwise, all of Peano's assertions
 * will fail. After that, we invoke the set's clone operation. This should now
 * be a deep copy. Again, we might be overambitious with creating new local
 * copies of data which are actually not local at all. We don't care. Such
 * data will be thrown away later.
 *
 *
 * ### General remarks
 *
 * Ensure that the underlying static routines T::send(),
 * T::receiveAndMerge(), T::storePersistently() and T::loadPersistently() all
 * return true. Otherwise, no data exchange will be triggered by Peano's trees.
 *
 *
 * ## MPI parallelisation
 *
 * As MPI introduces a distribute memory paradigm, MPI is intrinsically tied
 * to a deep copy data ownership policy.
 *
 * To realise the MPI data exchange, I did originally plan to require the
 * handed in vector datatype to provide some proper MPI datatypes. This did
 * not work: If we have inheritance, the byte alignment easily becomes messed
 * up and I never really managed to make everything work. Therefore, I do
 * work now with three different, explicit buffers:
 *
 * - There's a size meta data buffer which is a sequence of integers. Each
 *   entry stores how big the underlying vector is.
 * - There's a debug meta data buffer which I befill if and only if we work
 *   with meta data. It is a long sequence of doubles. Per stack entry, I
 *   hold 2*Dimensions data points: Dimensions entries encode the x value of
 *   the debug data, and a further Dimensions entries encode the h value.
 * - The actual data has to be flattened (serialised) too, as I cannot
 *   transfer any pointers. This serialised data is held in yet another
 *   buffer and has to be copied forth and back. Obviously, there's no need
 *   to use this buffer if no data travels in-between MPI boundaries.
 *
 * From a data ownership point of view, the startReceive() routine plays the
 * role of clone(), i.e. it creates new local copies. These will, from thereon,
 * be merged into the local data via shallow copies, i.e. by moving around
 * pointers to the deep copies.
 *
 * startSend() also copies data explicitly into its local buffer. We have to
 * delete those local copies in tryToFinishSendOrReceive(). Other than that,
 * nothing is to be done. The outgoing stack had pointers to objects on the
 * heap. We may assume that these objects still are used, and thus can simply
 * delete our own pointers without touching the stuff they point to.
 *
 * @see ParticleSet.template.h for an example of the required signature.
 */
template <class T>
class peano4::stacks::STDVectorOverVectorOfPointersStack: public peano4::stacks::STDVectorStack<T> {
  private:
    /**
     * Logging device.
     */
    static tarch::logging::Log _log;

    typedef peano4::stacks::STDVectorStack<T>   Base;

    #ifdef Parallel
    MPI_Request*           _metaDataSizeMPIRequest;
    MPI_Request*           _metaDataDebugMPIRequest;

    typename T::DoFType*   _deepCopyDataBuffer;
    int*                   _metaDataSizeBuffer;
    double*                _metaDataDebugBuffer;

    void workInReceivedMetaData();


    void workInReceivedData();


    void prepareDataToSendOut();

    /**
     * @return Total size of all entres
     */
    int prepareMetaDataToSendOut();

    #endif

  public:
    STDVectorOverVectorOfPointersStack();


    /**
     * Clone data into the current object on which clone() is called.
     *
     * Semi-shallow copy. We first create exactly the same number of stack
     * elements as data. After that, we invoke T's clone() per stack entry.
     * This is typically a shallow copy, though you might decide that not
     * all entries are to be copied.
     */
    void  clone( const STDVectorOverVectorOfPointersStack<T>&  data );


    /**
     * As we only have meta data, i.e. as our vectors just point to memory locations
     * on the heap, we simply call clear on each individual substack, i.e. forget the
     * actual pointers, and then we reset _currentElement such that we start to befill
     * our local stack from scratch again.
     */
    void clear();


    /**
     * See class documentation.
     */
    void startSend(peano4::grid::TraversalObserver::SendReceiveContext, int rank, int tag, MPI_Comm comm);


    /**
     * See class documentation.
     */
    void startReceive(peano4::grid::TraversalObserver::SendReceiveContext, int rank, int tag, MPI_Comm comm, int numberOfElements);


    /**
     * See class documentation.
     */
    bool tryToFinishSendOrReceive();
};


#include "STDVectorOverVectorOfPointersStack.cpph"



