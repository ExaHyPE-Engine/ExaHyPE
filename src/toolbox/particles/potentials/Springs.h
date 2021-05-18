// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TOOLBOX_PARTICLES_POTENTIALS_SPRINGS_
#define _TOOLBOX_PARTICLES_POTENTIALS_SPRINGS_


#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"


namespace toolbox {
  namespace particles {
    namespace potentials {
      /**
       * Assumes that there is no force in-between two particles p1
       * and p2 if they are h away from each other. If they are closer
       * than that, there's repulsion. Otherwise, we have attraction.
       * The force quantity is realised through an inverted Lennard-Jones
       * potential for the attraction, i.e. if two particles are slightly
       * too far away from each other, they attract each other. When they
       * are further away, this attraction also fades away. This way, we
       * can work with local support (cut-off radii) and we can handle
       * huge domains. In return, two particles repulse each other if they
       * are too close. However, I do truncate this repulsion, as I want
       * to ensure that these spring forces do not explode. So I kind of
       * truncate it in the interval [-1,+1].
       *
       * @param p1            Position of the particle of interest, i.e.
       *                      the one that is to be evaluated
       * @param p2            Position of comparison particle
       * @param h             This is the spacing particles should have
       *
       *
       * The potential's force should usually scale with the time step size,
       * but it is the job of the user of this routine to ensure proper
       * scaling.
       *
       * @return Vector from p1 to p2 representing a force.
       */
      tarch::la::Vector<Dimensions,double> springPotential(
        const tarch::la::Vector<Dimensions,double>& p1,
        const tarch::la::Vector<Dimensions,double>& p2,
        double h
      );
    }
  }
}

#endif
