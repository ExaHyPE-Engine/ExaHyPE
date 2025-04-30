// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/VertexEnumerator.h"
#include "tarch/la/Vector.h"


namespace toolbox {
  namespace particles {
    /**
     * Default tolerance to decide when two particles are the same.
     *
     * @see toolbox::particles::particleIsDuplicate()
     */
    constexpr double SpatialDuplicateTolerance = 1e-6;

    /**
     * Check if there's already a particle at p's position in the set.
     *
     * As we return a non-const iterator, the operation may not be labelled
     * as const. It is essential
     *
     *
     * @param particle You want to search the container identified by begin
     *   and end for this particle.
     * @return container's end() if there is no duplicate. Otherwise, return
     *   iterator pointing to existing particle.
     */
    template <typename Iterator>
    Iterator particleIsDuplicate(
      const typename Iterator::value_type& particle,
      Iterator                             begin,
      const Iterator&                      end
    );

    /**
     * Return true if and only if
     *
     * - we deal with a local particle (virtual ones are copies, to we are not
     *   really entitled to say something about them), and
     * - the particle resides within the computational domain.
     */
    template <typename Particle>
    bool particleWillStayWithinComputationalDomain(
      const Particle&                              particle,
      const tarch::la::Vector<Dimensions, double>& domainOffset,
      const tarch::la::Vector<Dimensions, double>& domainWidth
    );


    /**
     * Insert particle into a cell
     *
     * Shallow copy, i.e. you can remove the input container newParticles, but
     * you may not delete the particles it is pointing to.
     *
     * This routine basically runs through the vector passed and calls the
     * other insertParticleIntoCell() operation per particle.
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
     *
     * This routine assumes that the particle indeed fits into a cell.
     * Otherwise, I cannot assign in properly. If it falls into a cell,
     * then I can in return rightfully assume that the particle is local.
     * Setting the flag to local is important, as the locality analysis
     * of Peano usually sets particles to local in touchCellFirstTime().
     * You don't know if the particle insertion is called before or after
     * this analysis step, so we might just have missed the local insertion
     * and therefore miss out on particles.
     *
     * I recommend never to insert a particle directly, but always to run
     * through this routine and to combine it with a particle sorting step.
     * On the one hand, this ensures that
     * particles are inserted in a consistent way, i.e. stored next to their
     * closest neighbour. On the other hand, this ensures that a
     * particle is never assigned to a hanging vertex and then lost. If you
     * assign to a hanging vertex in a step where no grid re-sorting is active,
     * you will effectively loose the particle, as it will disappear with the
     * elimination of the hanging vertex.
     *
     * ## Realisation
     *
     * - Find which vertex in the cell is the closest vertex.
     * - Set the particle state to local. As we know that we insert only into
     *   local cells, we can do this without any fear.
     *
     * @see updateContainedInAdjacentCellProperty() which is called by this
     *   routine for newParticles.
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
     *
     * Each particle is associated with a vertex of a cell. This vertex is
     * identified by assignedVertex. We now compute a bitset that tells us,
     * from the vertex' point of view, which of the adjacent cells will
     * host this particle. This might not be unique, as we work with a
     * fuzzy geometric ownership.
     *
     * @see the explanation in Particle.py
     */
    void updateContainedInAdjacentCellProperty(
      const peano4::datamanagement::CellMarker& marker,
      const std::bitset<Dimensions>&            assignedVertex,
      auto&                                     particle
    );

    /**
     * We are
     */
    void updateContainedInAdjacentCellProperty(
      const peano4::datamanagement::VertexMarker& marker,
      auto&                                       particle
    );

    /**
     * Update internal marker of particle
     *
     * Each particle is associated to a vertex. The vertex is identified by
     * its position x and the surrounding cells with size h. Each vertex has
     * @f$ 2^d @f$ adjacent cells. The particle's
     * ContainedInAdjacentCellProperty bitset flags which of these adjacent
     * cells consider the particle to fall in their area.
     *
     * As we work with a weak ownership, there can be more than two bits (if
     * the particle sits exactly on the face between two adjacent cells).
     *
     *
     * ## Particle movement
     *
     * We have to call updateContainedInAdjacentCellProperty() after each
     * particle movement. Even if we don't change the vertex association as a
     * particle drifts, it might stay with the same vertex and still move from
     * one cell into another one and hence require an update of the flags.
     *
     * @see the explanation in toolbox.particles.Particle
     */
    void updateContainedInAdjacentCellProperty(
      const tarch::la::Vector<Dimensions, double>& x,
      const tarch::la::Vector<Dimensions, double>& h,
      auto&                                        particle
    );

    bool particleAssignedToVertexWillBeLocal(
      const tarch::la::Vector<Dimensions, double>& x,
      const peano4::datamanagement::VertexMarker&  marker
    );

    /**
     * Mirror a particle along the periodic domain boundaries.
     *
     * We check for every boundary whether it is periodic. Furthermore, we check
     * whether the particle would be already contained within a vertex along
     * each dimension. This is necessary for vertices in corners of the domain:
     * In these cases, we need to mirror the particle only along the boundary
     * that isn't contained in the vertex. Otherwise, the particle is displaced
     * too far away. E.g. For a vertex at (0, 0) a particle at (0.95, 0.) would
     * be displaced to (-0.05, -1.) instead of (-0.5, 0.).
     *
     * @param x Position of the particle as received along a periodic boundary
     *   exchange.
     * @param marker Marker of vertex which triggers the receive, i.e. the one
     *   to which the incoming particle at x will be attached to.
     *
     * @param Position of particle as "seen" from receiving side
     */
    tarch::la::Vector<Dimensions, double> mirrorParticleAlongPeriodicDomains(
      const tarch::la::Vector<Dimensions, double>& x,
      const peano4::datamanagement::VertexMarker&  marker,
      const tarch::la::Vector<Dimensions, double>  domainOffset,
      const tarch::la::Vector<Dimensions, double>  domainSize,
      const std::bitset<Dimensions>                periodicBC
    );

    /**
     * Applies the periodic boundary condition on particle coordinates `x`.
     *
     * In mirrorParticleAlongPeriodicDomains(), we check whether a vertex is
     * located on the boundary, and if it is, apply the boundary conditions
     * such that the vertex would "see" the particles.
     * In contrast, here we blindly apply the periodic boundary conditions
     * to a particle. This function is intended to be run on global particle
     * lists, such as is used e.g. in exchangeSieveListsGlobally().
     */
    tarch::la::Vector<Dimensions, double> applyPeriodicBoundaryConditions(
      const tarch::la::Vector<Dimensions, double>& x,
      const tarch::la::Vector<Dimensions, double>  domainOffset,
      const tarch::la::Vector<Dimensions, double>  domainSize,
      const std::bitset<Dimensions>                periodicBC
    );
  } // namespace particles
} // namespace toolbox


#include "particles.cpph"
