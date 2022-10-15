// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "peano4/datamanagement/VertexMarker.h"


namespace toolbox {
  namespace particles {
    typedef int ParticleReassociationInstruction;

    constexpr int ParticleReassociationInstruction_Keep          = -1;
    constexpr int ParticleReassociationInstruction_SieveGlobally = -2;

    /**
     * If you use this operation, you can be sure that every particle is
     * associated to the right vertex after we've finished the traversal, or
     * it is dumped into the set of particles that have to be sieved.
     * Therefore, you don't have to check any particle anymore in
     * touchVertexLastTime(). All should be in place.
     */
    ParticleReassociationInstruction getParticleReassociationInstructionWithinCellWithIntraCellReassignment(
    );


    /**
     * Should be called by touchVertexLastTime if you decide to sort particles
     * in this routine and not within the cell. It returns one of the following
     * constants:
     *
     * - ParticleReassociationInstruction_Keep. All is fine, just analyse the
     *   next particles.
     * - 0..2^d-1. The particle should not be associated to this vertex. Remove
     *   the association and associate to the next coarsest level. The vertex
     *   within this coarse cell is identified by the result number.
     * - ParticleReassociationInstruction_SieveGlobally. Particle should not be
     *   associated with this vertex, but you can't just move it one level up.
     *   Remove it completely from mesh and push it into a global list of
     *   particles which shall be sieved in the next step.
     *
     * ## What happens if a particle is on the wrong level
     *
     * We basically first check if a particle is contained within the h/2
     * environment of a vertex. For this, we exploit the fact that a marker
     * has a routine x() which returns the position of the associated of the
     * vertex plus a routine h() which returns the size of the adjacent cells.
     *
     *
  // If the parent cell is local, then we can safely move the particles
  // up the tree, as we know that all 2^d coarse grid vertices are
  // adjacent to the local domain. If the parent is not local, we have
  // to assume that we just did hit a vertical tree cut. In this case,
  // we are on the safe side if we just add the particles to the global
  // particle list.
     *
     */
    template <typename Particle>
    ParticleReassociationInstruction liftParticleAssociatedWithVertex(
      const Particle&                             p,
      const peano4::datamanagement::VertexMarker  marker
    );
  }
}


#include "MultiscaleTransitions.cpph"
