// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TOOLBOX_PARTICLES_PARTICLES_
#define _TOOLBOX_PARTICLES_PARTICLES_


#include "tarch/la/Vector.h"
#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/VertexEnumerator.h"


namespace toolbox {
  namespace particles {
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
      const tarch::la::Vector<Dimensions,double>&  domainOffset,
      const tarch::la::Vector<Dimensions,double>&  domainWidth
    );


    /**
     * Shallow copy, i.e. you can remove the input container newParticles, but
     * you may not delete the particles it is pointing to.
     */
    template <typename Particle, typename ParticleSet >
    void insertParticlesIntoCell(
      const peano4::datamanagement::CellMarker&                 marker,
      const std::vector< Particle* >&                           newParticles,
      peano4::datamanagement::VertexEnumerator< ParticleSet >&  fineGridVertices
    );
  }
}


#include "particles.cpph"


#endif
