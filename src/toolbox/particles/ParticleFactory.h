// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TOOLBOX_PARTICLE_PARTICLE_FACTORY_
#define _TOOLBOX_PARTICLE_PARTICLE_FACTORY_


#include <vector>
#include "peano4/utils/Globals.h"


namespace toolbox {
  namespace particles {

    /**
     * Init particle
     *
     * This is the most generic, minimalist initialisation of a particle that one can think of.
     * The minimal data we need per particle is
     *
     * - a position x which defines where the particle is located,
     * - a cut-off radius which defines on which resolution level to hold the particle.
     *
     * The further attribute that are set are
     *
     * - the move state which is set to New.
     * - the state is set to halo. That means, by default the particle is set to remote. But
     *   it will/should be set to local in the first sweep.
     *
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
     * @param h Spacing of particles. Pick it smaller or equal to 0 and we'll always
     *   create one particle in the center.
     * @param roundDown Always round down. In this case, you get a totally equal-spaced
     *   initial particle layout, but it will not match h globally.
     */
    template <class T>
    std::vector<T*> createEquallySpacedParticles(double h, const peano4::datamanagement::VertexMarker& marker, bool roundDown, bool addNoise);
  }
}


#include "ParticleFactory.cpph"


#endif
