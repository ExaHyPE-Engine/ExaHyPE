// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

/**
 * @page toolbox_particles_memorypool Particle memory pools
 *
 * This is a discussion around the data storage of particles within Peano.
 * Peano's particle toolbox works with pointers to individual particles on the
 * heap. It makes no assumptions where the particles are and simply moved
 * pointers around. By default, all creational routines allocate particles via
 * a simple new. This can lead to memory fragmentation. It also means that
 * compute kernels over lists of particles might induce scattered memory
 * accesses.
 *
 * The toolbox therefore offers action sets which take the particles
 * and consolidate them into larger, continuous chunks of memory. They employ
 * a memory pool. The idea of a memory pool is that we try to store (parts of)
 * the particles consecutively in memory. By default, Peano's particle
 * toolbox scatters data over the memory:
 *
 * @image html toolbox/particles/ParticleSetGenerator_ScatteredOnHeap.png
 *
 * You can alter the storage by picking a particular particle set generator.
 * Consult peano4.toolbox.particles.ParticleSet for details.
 *
 * The intention behind the continuous, compact storage is not only a more
 * efficient memory handling and a reduction of fragmentation. It also enables
 * the code to efficiently encode the active and local lists and, eventually,
 * to vectorise. toolbox.particles.UpdateParticle_MultiLevelInteraction_StackOfLists
 * is the prime example of such a tree walker which uses the fact that
 * particles are stored an bloc.
 *
 *
 * # Realisation
 *
 * There are different ways to realise memory pools. Originally, I
 * considered overloading the new and delete operator, i.e. to plug into the
 * native C++ memory management. In such a case, we would generate a C++
 * particle class, overwrite its generated new and delete operator, and
 * therefore bind the particle species to a memory pool. However, there were
 * all kind of issues when I studied this approach en detail, so I eventually
 * gave up on this idea.
 *
 * The new version now is a little bit more down to the ground: Each individual
 * vertex hosts a set of pointers administered through a memory pool object.
 * By default, its particles are not tied to pool, i.e. roam on th heap.
 * When I touch a vertex for the first time, I know a couple of facts:
 *
 * - This vertex data is now on the local call stack. It has been loaded from
 *   the input stack into the local call stack before Peano invokes
 *   touchVertexFirstTime(). Notably, this vertex data is not a copy on a
 *   communication stack and it had not yet been moved over into some other
 *   temporary stack.
 * - All the pointers that I host in the vertex are unique. This is likely
 *   the first and last time, when I know for sure that noone else holds
 *   references to my particle, too.
 *
 * I therefore can do a couple of things:
 *
 * 1. Make a vertex untied to the pool by default (default constructor).
 * 2. If the vertex is not yet tied to a memory pool entry and hosts some
 *    particles, reserve space for that many particles in the pool, copy the
 *    particles from the global heap over and make the individual pointers
 *    that the vertex holds point to these particles.
 * 3. Overwrite the insertion routines for the particle. If the user inserts
 *    a particle into a vertex which is untied, just add it to the list. If
 *    the vertex however is already tied to a memory pool, ensure that this
 *    memory pool is large enough, and copy the particle into this location.
 * 4. If a particle is extracted from a tied vertex, copy it into a real
 *    heap location first and then release the corresponding entry. We ensure
 *    that the particle is on the heap and the user can do whatever they
 *    want (and notably call delete) first and then we mark this memory
 *    location internally to be reused.
 *
 * We have to overwrite/redefine a few routines of a std::vector. However, the
 * C++ container offers quite a lot of ways to manipulate its entries. I don't
 * want to feature-proof all of them. Therefore, it is natural to use private
 * inheritance and to expose the routines of which I'm sure that they work
 * via wrappers.
 *
 * The most important exception is the fact that we do not(!) overwrite erase
 * but instead eliminate it from the signature. erase() is very dangerous. We
 * provide a routine grabParticle() which returns the "extracted" particle
 * which is actually now a copy from the vertex data onto the heap and removes
 * the pointer from the underlying vertex.
 *
 * If we erase data from a memory block tied to a vertex, it is important that
 * we do not alter the total underlying memory arrangement. Other vertices and
 * data blocks might have access to this one. We close the discussion with a
 * final observation which is over relevance for some codes:
 *
 * When we encounter touchVertexLastTime() we may not(!) assume that noone else
 * keeps the pointers to vertex data. Data might be used afterwards to lift
 * particles to other levels. I am not sure if this might be too pessimistic,
 * but one is definitely on the safe side not assuming anything about the
 * particle ordering in this event.
 *
 * By default, particles are managed via a particle set as produced by
 * peano4.toolbox.particles.ParticleSet. That is, it is this Python class which
 * decides which particle set flavour to produce. Please study
 *
 * - toolbox::particles::memorypool::VertexWiseContinuousMemoryPool
 * - toolbox::particles::memorypool::GlobalContinuousMemoryPool
 *
 * for examples of different pool realisations (in C++) and study the various
 * generators shipped with peano4.toolbox.particles.ParticleSet for further
 * flavours.
 *
 *
 * # Gather
 *
 * Enabling a particle pool is only the first step. You still have to ensure
 * that a mesh traversal takes the data scattered all over the heap and dumps
 * it into the memory pool. As we use a @ref page_toolbox_particles_mesh_storage "vertex-centred storage",
 * we have to ensure that the mesh traversal invokes gather() on the vertices;
 * and in return also calls scatter if particles leave their position in the
 * mesh aka memory.
 *
 * Within the Python API, you can use toolbox.particles.GatherParticlesInMemoryPool
 * to inject the gather behaviour. Scattering should be realised automatically,
 * as the particle set generator ensures that scatter is invoked every time you
 * lift of drop particles.
 *
 *
 * # Resorting
 *
 * You can trigger the resorting in various places. I do recommend to use
 * the parallel state update. Swift for example implements it there as
 * documented in swift2.graphcompiler.AMRLogic.add_dynamic_mesh_refinement_and_particle_resorting().
 *
 *
 * # Efficient compute kernels
 *
 * Efficient tree traversals which exploit the optimised storage are
 * peano4.toolbox.particles.UpdateParticle_MultiLevelInteraction_StackOfLists_ContiguousParticles and
 * peano4.toolbox.particles.UpdateParticle_SingleLevelInteraction_ContiguousParticles.
 * Please consult their documentation re what happens if data are scattered over the memory,
 * i.e. if vertices have not been gathered yet or if particles have changed their position
 * or cut-off radius.
 *
 */
