// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <list>
#include <unordered_set>


namespace swift2 {
  namespace kernels {
    namespace gravity{

    /**
     * Hard-coded two-body gravitational interaction for Earth-Sun system
     */
    template <typename ParticleContainer>
    void computeGravitationalForce(
        const ParticleContainer&  localParticles,
        const ParticleContainer&  activeParticles
        );

    }
  } 
} 

#include "TwoBodyProblem.cpph"
