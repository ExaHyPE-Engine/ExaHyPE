// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <list>


namespace swift2 {
  namespace kernels {
    /**
     * This routine runs over all the local particles and tries to
     * ensure that the number of particles equals roughly
     * targetNumberOfNeighbourParticles. The algorithm is fairly simple:
     *
     * - Find out how many other particles are currently within the cut-off
     *   radius.
     * - If this number is bigger than the target value, decrease the
     *   cut-off radius slightly. Count again. If we are still above the
     *   target value, accept the slightly reduces cut-off. Otherwise,
     *   undo the alteration.
     * - If the number of interaction partners is too small, increase it
     *   and study the effect:
     *   - If we meet the target neighbour count now, we are happy.
     *   - If we have increased the number of neighbours but are not there
     *     yet, we keep this increase, but ask Peano/Swift to run this
     *     analysis again.
     *   - If the increase has not paid off, we keep the old cut-off. It
     *     means that likely the interaction radii overall all are too
     *     small or the particle density is just not there.
     *
     *
     * The reduction starts from the assumption that we should be careful
     * with reducing the cut-off radii. So we only slightly decrease the
     * cut-off. If we could have reduced it more aggressively, we accept
     * that and hope that subsequent time steps or sweeps will eventually
     * bring the cut-off down. But we do not enforce it here, which might
     * mean that we work with too big interaction sets.
     *
     * The increase is different: If we
     *
     *
     * If an interaction radius
     * has to be increased, the code sets the flag
     * setRerunPreviousGridSweep() on the underlying particle
     * species.
     *
     * In prin
     *
     */
    template <typename Particle>
    void adoptInteractionRadiusAndTriggerRerun(
      const std::list<Particle *>&  localParticles,
      const std::list<Particle *>&  activeParticles,
      int                           targetNumberOfNeighbourParticles,
      double                        maxGrowthPerSweep = 2.0,
      double                        shrinkingFactor   = 0.8
    );
  }
}


#include "ParticleCutOffRadiusCalculation.cpph"

