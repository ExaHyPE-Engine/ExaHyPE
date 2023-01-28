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
 * Most datatypes will provide smart pointers and in return work with shallow
 * copies as data travel over the individual stacks. This implies that we "only"
 * have to redefine the clones which actually are deep copies.
 *
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
     * Deep clear
     *
     * Counterpart of clone(). We call clear() per stack entry, which
     * in itself is another vector. So it is up to you to decide if
     * you want to have a shallow or a deep clear().
     */
//    void clear();
};


#include "STDVectorStackOverSmartPointers.cpph"



