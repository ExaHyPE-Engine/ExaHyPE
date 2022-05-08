// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TOOLBOX_PARTICLES_PARTICLES_
#define _TOOLBOX_PARTICLES_PARTICLES_


#include "tarch/la/Vector.h"


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
  }
}


#include "particles.cpph"


#endif
