// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <vector>
#include "peano4/utils/Globals.h"

#include "peano4/datamanagement/VertexMarker.h"
#include "peano4/datamanagement/CellMarker.h"


namespace toolbox {
  namespace particles {

    /**
     * Init particle
     *
     * This is the most generic, minimalist initialisation of a particle that
     * one can think of. The minimal data we need per particle is
     *
     * - a position x which defines where the particle is located,
     * - a cut-off radius which defines on which resolution level to hold the
     *   particle.
     *
     * The further attribute that are set are
     *
     * - the parallel state is set to Local, so we assume that you only insert
     *   meaningful data into the grid.
     *
     * It is important to make particles local initially. All of Peano's sorting
     * only works with local particles, i.e. if you initially set particles to
     * virtual then the sorting will ignore them. This is not what we want.
     *
     *
     * ## Parallel lifecycle
     *
     * The parallel lifecycle is handled via the Python action set
     * UpdateParallelState. This mapping is relatively simple: It initially
     * sets the (new) parallel state of every single particle to virtual, and
     * then marks the particle as local if it resides within a local cell. At
     * the end of the traversal, we look at this new parallel state: If it had
     * been virtual before and it is still virtual, then we throw it away. This
     * particle clearly does not fall into the local domain.
     *
     * If I initialise my grid improperly, i.e. insert particles into a cell
     * which are actually not within the cell (you can always do this by
     * associating a particle with a cell's vertex and then adding some noise,
     * e.g.), then I end up with situations where I shouldn't have inserted
     * this particle at all. By setting every new particle to virtual, the new
     * parallel state of such an "invalid" particle will be virtual, too, and
     * the mapping consequently erases the particle immediately.
     *
     * This "by default virtual"-policy therefore implements a confirmation
     * mechanism: We insert a new particle which is atm virtual and usually
     * won't be updated. We wait for the first grid sweep for the code to
     * confirm that it is actually inside and we should work with it. If it
     * does not confirm, we throw it away immediately towards the end of the
     * first sweep after the insertion.
     *
     * We finally conclude this discussion with the observation that we only
     * have to set the particle's current state. The new state is used by the
     * action set UpdateParallelState and will be overwritten later anyway.
     * However, just to be on the safe side, we set it to virtual. This way,
     * we ensure that the lifecycle is all correct even if we insert particles
     * on the fly. UpdateParallelState should always be the last thing we do
     * when we leave a cell.
     */
    template <class T>
    void init(
      T&                                          particle,
      const tarch::la::Vector<Dimensions,double>& x,
      double                                      cutOffRadius
    );


    /**
     * Typical usage of this factor method:
<pre>
if ( not marker.isRefined() ) {
  auto newParticles = toolbox::particles::createEquallySpacedParticles<{{PARTICLE}}>({{H}},marker);
  fineGridVertex{{PARTICLES_CONTAINER}}.insert( fineGridVertex{{PARTICLES_CONTAINER}}.end(), newParticles.begin(), newParticles.end() );
}
</pre>

     * @param spacingH Spacing of particles. Pick it smaller or equal to 0 and we'll always
     *   create one particle in the center.
     *
     * @param voxelX Centre of voxel in space
     *
     * @param voxelH Size of voxel, i.e. subcell, into which the particles shall be embedded
     *
     * @param roundDown Always round down. In this case, you get a totally equal-spaced
     *   initial particle layout, but it will not match h globally.
     *
     *
     */
    template <class T>
    std::vector<T*> createEquallySpacedParticles(double spacingH, const tarch::la::Vector<Dimensions,double>& voxelX, const tarch::la::Vector<Dimensions,double>& voxelH, bool roundDown, bool addNoise);

    template <class T>
    std::vector<T*> createParticlesAlignedWithGlobalCartesianMesh(double spacingH, const tarch::la::Vector<Dimensions,double>& voxelX, const tarch::la::Vector<Dimensions,double>& voxelH, const tarch::la::Vector<Dimensions,double>& domainOffset, bool addNoise);
  }
}


#include "ParticleFactory.cpph"


