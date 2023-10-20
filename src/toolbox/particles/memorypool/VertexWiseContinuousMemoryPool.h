// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


namespace toolbox {
  namespace particles {
    namespace memorypool {
      template <class T>
      struct VertexWiseContinuousMemoryPool;
    } // namespace memorypool
  }   // namespace particles
} // namespace toolbox


/**
 * Memory pool offering continuous global memory for a particle species
 *
 * Consult the @ref toolbox_particles_memorypool "generic description of memory pools" before
 * you continue to read this documentation. This class is used by every "vertex" object, i.e.
 * each vertex holds one instance of the pool object. Basically, the field container is used
 * to point to all the particles scattered over the memory after we've created the particles.
 * However, whenever we call gather(), we take all of these particles from the heap, put them 
 * into one continuous container and then make the pointers within container point to those
 * guys.
 * 
 * 
 */
template <class T>
struct toolbox::particles::memorypool::VertexWiseContinuousMemoryPool {
  public:
    typedef std::list<T*>  Container;

    Container  container;

    VertexWiseContinuousMemoryPool();

    void scatter();
    typename Container::iterator scatterAndUpdateIterator( const typename Container::iterator&  p );

    /**
     * Gather the particle
     *
     * If we invoke this routine on a gathered memory pool, it becomes nop.
     * Otherwise, the routine's implementation is rather straightforward:
     *
     * - Allocate one big chunk of memory
     * - Run over the list of pointers. This run-through has to be done with
     *   a reference, as we will change the pointers on our way. So we use
     *   auto& as type.
     * - Copy the current iterator element over into the big chunk of memory
     *   and increment the counter in there.
     * - Delete the original piece of data on the global heap.
     * - Make the list pointer point to the new fragment within the memory.
     *
     * Please note that it makes no sense to gather an empty set, so we
     * explicitly take that into account as well.
     */
    void gather();

    /**
     * Is the vertex data gathered
     *
     * This routine returns false if the underlying container is empty.
     */
    bool isGathered() const;

    /**
     * Replace particle
     *
     * Takes the particle identified via p and copies the content of newCopy
     * over.
     *
     * If our data is gathered, we copy stuff over. After that, newCopy is
     * deleted. If our data is scattered, we can just delete the original one
     * and push back the new copy.
     */
    void replace( typename Container::iterator p, T* newCopy );

    /**
     * Clears the underlying list of pointers and resets _gatheredDataPointer
     * to nullptr. The routine does not free the memory. This routine is
     * used by shallow clears().
     */
    void clearAndReset();
  private:
    static tarch::logging::Log  _log;

    /**
     * Is nullptr as long as data is not gathered.
     */
    T* _gatheredDataPointer;
};


#include "VertexWiseContinuousMemoryPool.cpph"

