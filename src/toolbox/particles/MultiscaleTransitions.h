// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "internal.h"
#include "particles.h"

#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/VertexMarker.h"

namespace toolbox {
  namespace particles {
    typedef int ParticleReassociationInstruction;

    constexpr int ParticleReassociationInstruction_Keep          = -1;
    constexpr int ParticleReassociationInstruction_SieveGlobally = -2;

    /**
     * Take particle from current vertex and lift it or keep it local
     */
    template <typename Particle>
    ParticleReassociationInstruction
    liftParticleAssociatedWithVertex(const Particle& p, const peano4::datamanagement::VertexMarker marker);

    /**
     * Implementation of liftParticleAssociatedWithVertex() without the template
     */
    ParticleReassociationInstruction liftParticleAssociatedWithVertex(
      bool                                         isLocal,
      double                                       searchRadius,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const peano4::datamanagement::VertexMarker   marker
    );

    /**
     * Decide whether a particle is sieved into a vertex
     */
    template <typename Particle>
    bool sieveParticle(const Particle& particle, const peano4::datamanagement::VertexMarker& marker);

    bool sieveParticle(
      double                                       searchRadius,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const peano4::datamanagement::VertexMarker&  marker
    );

    template <typename Particle>
    bool dropParticle(const Particle& particle, const peano4::datamanagement::VertexMarker& marker);

    /**
     * Alternative version of dropParticle() without templates
     */
    bool dropParticle(
      double                                       searchRadius,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const peano4::datamanagement::VertexMarker&  marker
    );

    /**
     * Will the particle be dropped further throughout the traversal
     */
    template <typename Particle>
    bool particleWillBeDroppedFurther(const Particle& particle, const peano4::datamanagement::CellMarker& marker);

    bool particleWillBeDroppedFurther(double searchRadius, const peano4::datamanagement::CellMarker& marker);

    /**
     * Will a particle be dropped further
     */
    template <typename Particle>
    bool particleWillBeDroppedFurther(const Particle& particle, const peano4::datamanagement::VertexMarker& marker);

    bool particleWillBeDroppedFurther(double searchRadius, const peano4::datamanagement::VertexMarker& marker);

    /**
     * Ownership tolerance
     */
    double relativeSpatialOwnershipTolerance(const ::peano4::datamanagement::CellMarker& marker);

    /**
     * Reassociate a particle within its cell or lift it
     */
    template <typename Particle>
    ParticleReassociationInstruction getParticleReassociationInstructionWithinCellWithIntraCellReassignment(
      const Particle&                           p,
      const peano4::datamanagement::CellMarker& marker,
      int                                       numberOfVertexWithinCell
    );

    ParticleReassociationInstruction getParticleReassociationInstructionWithinCellWithIntraCellReassignment(
      bool                                         isLocal,
      double                                       searchRadius,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const peano4::datamanagement::CellMarker&    marker,
      int                                          numberOfVertexWithinCell
    );

    /**
     * Find out which adjacent cell will hold a particle.
     */
    std::bitset<TWO_POWER_D> getAdjacentCellsOwningParticle(
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const peano4::datamanagement::VertexMarker&  marker
    );
  } // namespace particles
} // namespace toolbox

#include "MultiscaleTransitions.cpph"
