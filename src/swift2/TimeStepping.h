// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <list>
#include <vector>


namespace swift2 {
  /**
   *
   */
/*
  template <typename Particle>
  concept ParticleExplicitTimeSteppingAccessor = requires(const Particle& p){
//    typename ParticleAccessor::Particle;
    { x( p ) } -> tarch::la::Vector<Dimensions,double>;
  };
*/



  template <typename Particle>
  class TrivialParticleExplicitTimeSteppingAccessor {
    static tarch::la::Vector<Dimensions,double> x(const Particle& particle) {
      return particle.x();
    }

    static double getTimeStepSize() {
      return 0.1;
    }
  };



  /**
   * Takes a local list of particles and applies the explicit Euler to
   * them. Afterwards, it computes the maximum velocity over the set. The
   * latter information can be used to determine the CFL condition, e.g.
   *
   * We do not directly access the particle, but we expect an accessor
   * object. This is a concept: The accessor object has to provide the
   *
   */
  // das Typename sollte wohl ein concept sein
  template <typename Particle>
  void computeExplicitEulerWithGlobalTimeStepSizeAndReduceNewMaximalVelocity(
    const std::list< Particle* >&  localParticles
  );


  template <typename Particle>
  void resetParticleMarker(
    const std::vector< Particle* >&  localParticles
  );
}


#include "TimeStepping.cpph"


