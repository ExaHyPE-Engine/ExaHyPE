// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TOOLBOX_PARTICLE_PARTICLE_FACTORY_
#define _TOOLBOX_PARTICLE_PARTICLE_FACTORY_


#include <vector>


namespace toolbox {
  namespace particles {
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
     */
    template <class T>
    std::vector<T*> createEquallySpacedParticles(double h, const peano4::datamanagement::VertexMarker& marker);
  }
}


#include "ParticleFactory.cpph"


#endif
