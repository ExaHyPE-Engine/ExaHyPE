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
     *
     * We return
     *
     * - ParticleReassociationInstruction_Keep if the particle should
     *   remain where it is.
     * - A value between 0 and 2^d-1 if we should assign it to another
     *   vertex of this cell (on the same level).
     *
     */
    template <typename Particle>
    ParticleReassociationInstruction getParticleReassociationInstructionWithinCellWithIntraCellReassignment(
      const Particle&                           p,
      const peano4::datamanagement::CellMarker  marker,
      int                                       numberOfVertexWithinCell
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
     * If the particle is at the right place, we return ParticleReassociationInstruction_Keep.
     * Nothing is to be done. Otherwise, we have to lift:
     *
     * If the parent cell is local, then we can safely move the particles
     * up the tree, as we know that all 2^d coarse grid vertices are
     * adjacent to the local domain.
     *
     * If the parent cell is not local, we have
     * to assume that we just did hit a vertical tree cut. In this case,
     * we are on the safe side if we just add the particles to the global
     * sieve particle list. We return ParticleReassociationInstruction_SieveGlobally.
     *
     * All of these considerations apply if and only if a particle is local. If
     * a particle is virtual, it is a copy from another rank. In this case, we
     * should not change its position, and we should not change its association.
     */
    template <typename Particle>
    ParticleReassociationInstruction liftParticleAssociatedWithVertex(
      const Particle&                             p,
      const peano4::datamanagement::VertexMarker  marker
    );


    /**
     * Find out which vertex should hold a particle.
     *
     * @param particle The particle of interest to be studied.
     * @param marker   A description of the cell into which the particle is
     *                 currently inserted or shall be inserted.
     * @return         The number of the vertex within the cell identified by
     *                 marker to which you should assign particle. Use
     *                 to_ulong() to convert the result, as the result
     *                 identifies the vertex in z-order, i.e. if it returns 00
     *                 it is the left bottom vertex, 10 is the right bottom
     *                 one, and so forth.
     */
    template <typename Particle>
    std::bitset<Dimensions>  getParticleAssociationWithinCell(
      const Particle&                             particle,
      const peano4::datamanagement::VertexMarker  marker
    );


    /**
     * A particle is to be sieved into a vertex if
     *
     * - this vertex is closer to the particle than half the mesh size of
     *   an adjacent mesh element, as we always store particles next to
     *   their closest vertex; AND
     * - this vertex's cut-off radius would not fit into the next finer
     *   mesh level OR
     *   there is no next finer mesh level; AND
     * - the particle's cut-off is not too big for teh current mesh level.
     *
     */
    template <typename Particle>
    bool sieveParticle(
      const Particle&                             particle,
      const peano4::datamanagement::VertexMarker  marker
    );
  }
}


#include "MultiscaleTransitions.cpph"
