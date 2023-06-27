// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


namespace toolbox {
  namespace particles {
    namespace memorypool {
      template <class T>
      class GlobalContinuousMemoryPool;
    } // namespace memorypool
  }   // namespace particles
} // namespace toolbox


/**
 * Memory pool offering continuous global memory for a particle species
 *
 * Consult the @ref toolbox_particles_memorypool "generic description of memory pools" before
 * you continue to read this documentation. The pool comes along in a tandem
 * with a flavour of std::vector which is tied to the vertices. It holds a
 * reference to the pool obtained via getUniquePoolReference() or a
 */
template <class T>
class toolbox::particles::memorypool::GlobalContinuousMemoryPool {
public:
  class ParticleSet {};


  static constexpr int UndefinedPoolReference = -1;

  /**
   * Factory mechanism
   *
   * To be called within touchVertexFirstTime() if you want to link a vertex
   * to a memory bank in the pool.
   */
  int createUniquePoolReference();

  /**
   * Free a reference
   *
   * You can either pass in a value obtained via createUniquePoolReference()
   * or you pass in UndefinedPoolReference. Should be called by
   * destroyPersistentVertex() but if and only if all particles have
   * previously been removed from the pool.
   */
  void destroyUniquePoolReference(int reference);

  void removeParticleFromPool(int reference, pointer);

private:
};
