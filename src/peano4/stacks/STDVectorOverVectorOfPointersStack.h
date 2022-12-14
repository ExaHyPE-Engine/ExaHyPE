// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _PEANO4_STACKS_STDVECTOR_POINTER_STACK_H_
#define _PEANO4_STACKS_STDVECTOR_POINTER_STACK_H_


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
 * I assume that T is in itself a vector over pointers with some additional
 * routines beyond the ones offered by the STL. This class therefore implements a
 * stack as vector over a pimped vector of pointers.
 *
 * The list of additional routines or routines with special semantics that I
 * need from T are
 *
 * - clone() - through this method, it is up to the implementation of T to decide
 *   if you want to work with shallow or deep copies. I strongly recommend deep
 *   copies as per documentation below.
 * - clear() - counterpart of clone(). If you use the standard std::vector's
 *   clear(), you'll end up with a shallow clear, as you simply erase the list
 *   over pointers. Again, up to the implementation to decide what to use, but
 *   some remarks are blow.
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
 * Views have to be supported by this specialised vector, but I get this for
 * free a I inherit from the other vector class.
 *
 *
 * ## Data ownership (on single tree/subpartition)
 *
 * The vectors don't actually own any data for this particular stack type.
 * The data resides on the heap and the vectors are mere meta data structures
 * referencing the core data.
 *
 * The stack class therefore works with shallow copies by default.
 *
 *
 * ### Intra-node parallelisation
 *
 * The std::vector copy constructor copies the pointers to the particles
 * when we create a new object, while a push on two stacks pushes two
 * pointers to the same stack to the respective containers. Consequently,
 * any particle along the boundary is pushed multiple times: To its actual
 * container, but also to the vectors along the boundary.
 *
 * When we clone a vector along the boundary, it is thus important that this
 * clone manually creates a deep copy. When we merge, merge() takes the
 * deep copy and inserts it into the destination tree (where teh underlying
 * data is marked as ghost and consequently deleted).
 *
 *
 * ### Periodic boundary conditions
 *
 * Same as above. Data is deep copied and then inserted on the other side.
 *
 *
 * ### MPI
 *
 * A vertex at a domain boundary pipes its data onto different stacks: the
 * local stack and one stack per communication partner. These communication
 * partner stacks hold shallow copies. So far, nothing is copied really from
 * Peano's point of view. The data administered through this outgoing stack
 * not only is a shallow copy, its pointers also point to scattered data.
 *
 * To send out a stack, we thus first have to deep copy the whole content
 * into a continuous buffer, send this buffer out, and eventually destroy
 * the deep copy. If we receive, we have to create a continuous buffer,
 * and receive into this one. When we "unpack" the data into a vector of
 * vectors, we don't have to destroy the original buffer. We have to store
 * the particels somewhere in the end. However, this causes problems if we
 * later on eliminate particular subsegments (some particles) or more elements
 * around: We cannot delete parts of one large memory block which we previously
 * allocated en bloc.
 *
 *
 * ### Deep copies, data ownership and memory deletes
 *
 * The clear() operation on the subclass is a shallow clear, i.e. it only
 * eliminates the pointers but does not delete any actual data. We expect
 * the actual grid traversal to throw away remote content after the grid
 * sweep. See the comments above.
 *
 *
 *
 * ### General remarks
 *
 * Ensure that the underlying static routines T::send(),
 * T::receiveAndMerge(), T::storePersistently() and T::loadPersistently() all
 * return true. Otherwise, no data exchange will be triggered by Peano's trees.
 *
 *
 *
 * @see ParticleSet.template.h for an example of the required signature.
 *
 * @param T Is usually a std::vector over pointers or any class providing a
 *   similar signature.
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
     * This operation overwrites the baseclass version. If we called the
     * plain base class version, we would end up with a shallow copy.
     * However, we want a semi-shallow copy: We replicate the meta data,
     * and then we call the clone() of the actual vector over pointers.
     * It is up to this routine if we craete a shallow copy or not.
     *
     * Read the class documentation why I think that the clone() on the
     * subvectors should create a deep copy. It is the section on
     * intra-node parallelisation.
     */
    void  clone( const STDVectorOverVectorOfPointersStack<T>&  data );


    /**
     * Deep clear
     *
     * Counterpart of clone(). We call clear() per stack entry, which
     * in itself is another vector. So it is up to you to decide if
     * you want to have a shallow or a deep clear().
     */
    void clear();


    /**
     * See class documentation.
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
     */
    void startSend(peano4::grid::TraversalObserver::SendReceiveContext, int rank, int tag, MPI_Comm comm);


    /**
     * See class documentation.
     *
 * To send out a stack, we thus first have to deep copy the whole content
 * into a continuous buffer, send this buffer out, and eventually destroy
 * the deep copy. If we receive, we have to create a continuous buffer,
 * and receive into this one. When we "unpack" the data into a vector of
 * vectors, we don't have to destroy the original buffer. We have to store
 * the particels somewhere in the end. However, this causes problems if we
 * later on eliminate particular subsegments (some particles) or more elements
 * around: We cannot delete parts of one large memory block which we previously
 * allocated en bloc.
     *
     */
    void startReceive(peano4::grid::TraversalObserver::SendReceiveContext, int rank, int tag, MPI_Comm comm, int numberOfElements);


    /**
     * See class documentation.
     */
    bool tryToFinishSendOrReceive();
};


#include "STDVectorOverVectorOfPointersStack.cpph"



#endif
