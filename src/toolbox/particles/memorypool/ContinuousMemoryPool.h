// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once



namespace toolbox {
  namespace particles {
    namespace memorypool {
      template <class T>
      class ContinuousMemoryPool;
    }
  }
}



/**
 * Memory pool offering continuous global memory for a particle species
 *
 * Peano's particle toolbox works with pointers to individual particles on the
 * heap. It makes no assumptions where the particles are and simply moved
 * pointers around. By default, all creational routines allocate particles via
 * a simple new. This can lead to memory fragmentation. It also means that
 * compute kernels over lists of particles might induce scattered memory
 * accesses. The toolbox therefore offers action sets which take the particles
 * and consolidate them into larger, continuous chunks of memory. They employ
 * a memory pool.
 *
 * All of Peano's memory pools realise a first-particle rules policy. That is,
 * if you have N particles associated with a vertex, the first particle
 * determines where the N-1 particles are actually stored.
 *
 * The global memory pool works with one large continuous memory.
 *
 * Do we have a unique number for particles?
 */
template <class T>
class toolbox::particles::memorypool::ContinuousMemoryPool {

};
