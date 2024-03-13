// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <vector>

#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/VertexMarker.h"
#include "peano4/utils/Globals.h"


namespace toolbox {
  namespace particles {

    /**
     * Init particle
     *
     * This is the most generic, minimalist initialisation of a particle that
     * one can think of. The minimal data we need per particle is
     *
     * - a position x which defines where the particle is located,
     * - a search radius which defines on which resolution level to hold the
     *   particle.
     *
     * The further attribute that is set is the parallel state. At this point,
     * we set it to Virtual. This highlights that fact that you should not
     * add particles directly to the mesh, but instead use
     * insertParticleIntoCell() which then in turn will toggle to Local.
     */
    template <class T>
    void init(T& particle, const tarch::la::Vector<Dimensions, double>& x, double searchRadius);

    /**
     * Create equally spaced particles for one cell
     *
     * This routine creates equally spaced particles within one cell. It is a
     * solely cell-local factory method, i.e. it does not take any adjacent
     * cell into account. As a consequence, you will get a locally equidistant
     * layout of particles, but you will not get a globally equidistant layout:
     * The particles will not be spaced the same way across a cell face as
     * they are spaced out inside a cell. If you need a globally equidistant
     * layout, you will have to use createParticlesAlignedWithGlobalCartesianMesh().
     *
     * The routine creates all the resulting particles on the heap via a plain
     * new(). That is, the particles will be scattered in memory.
     *
     * @param spacingH Spacing of particles. Pick it smaller or equal to 0 and we'll always
     *   create one particle in the center.
     *
     * @param voxelX Centre of voxel in space
     *
     * @param voxelH Size of voxel, i.e. subcell, into which the particles shall be embedded
     *
     */
    template <class T>
    std::vector<T*> createEquallySpacedParticles(
      double                                       spacingH,
      const tarch::la::Vector<Dimensions, double>& voxelX,
      const tarch::la::Vector<Dimensions, double>& voxelH,
      bool                                         addNoise
    );

    /**
     * Insert particles that are aligned along a Cartesian grid globally
     *
     * ## Biased insertion
     *
     * We compute the left number of the particle and the right one. If the
     * left particle ends up exactly on the face, then we add it. If the right
     * one ends up exactly on the face, we skip it. The rationale is that the
     * right neighbour will insert that guy later on, and we don't want to
     * insert particles twice.
     */
    template <class T>
    std::vector<T*> createParticlesAlignedWithGlobalCartesianMesh(
      double                                       spacingH,
      const tarch::la::Vector<Dimensions, double>& voxelX,
      const tarch::la::Vector<Dimensions, double>& voxelH,
      const tarch::la::Vector<Dimensions, double>& domainOffset,
      const tarch::la::Vector<Dimensions, double>& domainSize,
      bool                                         addNoise
    );
  } // namespace particles
} // namespace toolbox


#include "ParticleFactory.cpph"
