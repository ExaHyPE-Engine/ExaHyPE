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
    class STDVectorStackOverSmartPointers;
  }
}


/**
 * This stack hosts data which are stored on the heap and subject to smart pointers
 *
 * Once we work with smart pointers, we can work with shallow copies as data
 * travel over the individual stacks. This implies that we "only" have to
 * make the clones actual deep copies.
 *
 * We assume that T defines a pointer data which actually points to the data.
 * Most T variants will wrap this one in a smart pointer.
 *
 *
 * ## MPI
 *
 * The most tricky part in the context of smart pointers is actually the MPI
 * data exchange which has to take into account that data are scattered over
 * the heap. See the documentation of the send/receive operations. They discuss
 * how the business with the required deep copies actually works. Anyway, to
 * make it work, we need the used datatype T to have two fields:
 *
 * - A type expression DoFType;
 * - A constexpr int Cardinality.
 *
 *
 * @param T Hosts usually pointer which is backed up by a smart pointer, i.e.
 *   deleted as soon as noone needs it anymore. T has to provide a clone()
 *   operation.
 */
template <class T>
class peano4::stacks::STDVectorStackOverSmartPointers: public peano4::stacks::STDVectorStack<T> {
  private:
    /**
     * Logging device.
     */
    static tarch::logging::Log _log;

    typedef peano4::stacks::STDVectorStack<T>   Base;

    /**
     * See MPI routines below.
     */
    typename T::DoFType*   _gatheredData;
    double*                _gatheredMetaData;

    #ifdef Parallel
    MPI_Request*        _ioMetaDataMPIRequest;
    #endif

  public:
    STDVectorStackOverSmartPointers();


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
    void  clone( const STDVectorStackOverSmartPointers<T>&  data );


    /**
     * Start the actual send process
     *
     * Always pairs up with a finish... call.
     *
     * # Gathering the data
     *
     * As we have to assume that the data of interest are scattered over the
     * main memory, we first allocate one big buffer for the data. This is
     * a buffer over T::DoFType. We then copy the chunks over and send this
     * block rather than the "original" stack data.
     *
     * The meta data has to be treated (almost) in the same way: In this case,
     * we create manually a buffer over 2 * Dimensions * no of elements doubles
     * and then copy the meta data over one by one. That means: we do not
     * expect T's datatype to encapsulate the meta data - which also doesn't
     * make any sense as T's data itself is scattered over the memory since it
     * works with pointers.
     *
     *
     * # Cleaning up
     *
     * All clean ups are realised within tryToFinishSendOrReceive() where I
     * have to delete the gathered buffer. To save memory overall, I can delete
     * the pointers to the data scattered over the memory straightaway once all
     * is copied over. As these pointers are shared smart pointers, this
     * immediate delete might free memory as the shared pointer counter reduces
     * to zero.
     */
    void startSend(peano4::grid::TraversalObserver::SendReceiveContext context, int rank, int tag, MPI_Comm comm);


    /**
     * Counterpart of startSend()
     *
     * We have to prepare the en bloc, raw data buffers for both the data and
     * the meta data (if required). After that, we can trigger the two
     * non-blocking receives.
     *
     * @see startSend()
     */
    void startReceive(peano4::grid::TraversalObserver::SendReceiveContext context, int rank, int tag, MPI_Comm comm, int numberOfElements);


    /**
     * Semantics are alost an exact 1:1 copy of the superclass implementation
     *
     * ## Clean up
     *
     * If a send or receive had been pending and just completed, we start the
     * actual clean up process.
     *
     * As we know that MPI sends in order, we do not have to test for the meta
     * data. Once the real data has dropped in, we know that the meta data are
     * in place, too.
     *
     *
     *
     *
     * ## Rationale
     *
     * I originally wanted to call the superclass and just free the buffer if
     * its result is true. However, at the time when the result comes back, I
     * do not know if it has just switched from false to true or had been true
     * all along. To avoid complicated logic, I decided to replicate the parent
     * data and to inject the free into the replicated code.
     */
    bool tryToFinishSendOrReceive();
};


#include "STDVectorStackOverSmartPointers.cpph"



