// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/VertexEnumerator.h"
#include "peano4/datamanagement/VertexMarker.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"

namespace toolbox {
  namespace particles {
    /**
     * Default tolerance to decide when two particles are the same.
     *
     * @see toolbox::particles::particleIsDuplicate()
     */
    constexpr double SpatialDuplicateTolerance = 1e-6;

    /**
     * Check if there is already a particle at the position of particle.
     */
    template <typename Iterator>
    Iterator particleIsDuplicate(const typename Iterator::value_type& particle, Iterator begin, const Iterator& end);

    /**
     * Check if a particle is local and inside the computational domain
     */
    template <typename Particle>
    bool particleWillStayWithinComputationalDomain(
      const Particle&                              particle,
      const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
      const tarch::la::Vector<DIMENSIONS, double>& domainWidth
    );

    /**
     * Insert particle into a cell
     */
    template <typename Particle, typename ParticleSet>
    void insertParticlesIntoCell(
      const peano4::datamanagement::CellMarker&              marker,
      const std::vector<Particle*>&                          newParticles,
      peano4::datamanagement::VertexEnumerator<ParticleSet>& fineGridVertices,
      int                                                    spacetreeId
    );

    /**
     * Insert particle into cell
     */
    template <typename Particle, typename ParticleSet>
    void insertParticleIntoCell(
      const peano4::datamanagement::CellMarker&              marker,
      Particle*                                              newParticles,
      peano4::datamanagement::VertexEnumerator<ParticleSet>& fineGridVertices,
      int                                                    spacetreeId
    );

    /**
     * Update helper flag per particle for cell
     */
    template <typename Particle>
    void updateContainedInAdjacentCellProperty(
      const peano4::datamanagement::CellMarker& marker,
      const std::bitset<DIMENSIONS>&            assignedVertex,
      Particle&                                 particle
    );

    template <typename Particle>
    void updateContainedInAdjacentCellProperty(const peano4::datamanagement::VertexMarker& marker, Particle& particle);

    /**
     * Update internal marker of particle
     */
    template <typename Particle>
    void updateContainedInAdjacentCellProperty(
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      Particle&                                    particle
    );

    bool particleAssignedToVertexWillBeLocal(
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const peano4::datamanagement::VertexMarker&  marker
    );

    /**
     * Mirror a particle along the periodic domain boundaries.
     */
    tarch::la::Vector<DIMENSIONS, double> mirrorParticleAlongPeriodicDomains(
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const peano4::datamanagement::VertexMarker&  marker,
      const tarch::la::Vector<DIMENSIONS, double>  domainOffset,
      const tarch::la::Vector<DIMENSIONS, double>  domainSize,
      const std::bitset<DIMENSIONS>                periodicBC
    );

    /**
     * Applies the periodic boundary condition on particle coordinates `x`.
     */
    tarch::la::Vector<DIMENSIONS, double> applyPeriodicBoundaryConditions(
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>  domainOffset,
      const tarch::la::Vector<DIMENSIONS, double>  domainSize,
      const std::bitset<DIMENSIONS>                periodicBC
    );
  } // namespace particles
} // namespace toolbox

#include "particles.cpph"
