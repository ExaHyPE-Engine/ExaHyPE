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
  void computeExplicitEulerWithGlobalTimeStepSize(
    const std::list< Particle* >&  localParticles
  );

/*
  template <typename Particle>
  void computeExplicitEulerWithGlobalTimeStepSizeAndReduceNewMaximalVelocity(
    const std::list< Particle* >&  localParticles
  );
*/


  /**
   * Reset the marker of a particle from whatever it has been before to
   * NotMoved. This routine should be called in any task graph before a
   * particle's position is actually altered. The name is not brilliant,
   * as you should also call this routine prior to a change of the interaction
   * radius.
   */
  template <typename Particle>
  void resetMovedParticleMarker(
    const std::vector< Particle* >&  localParticles
  );


  template <typename Particle>
  void reduceVelocityAndCutOffRadius(
    const std::list< Particle* >&  localParticles
  );


  /**
   * Determine a total admissible time step size which is determined through
   * the maximum velocity within the mesh and the minimum cell size. It is
   * also subject to a CFL constant obviously. While every species has an
   * admissible time step size, not every species type (time integrator) might
   * use this one.
   *
   * If the maximum velocity in the system is zero, I set the time step size
   * to the initial or default time step size. This happens for example in the
   * very first time step, where we don't know the maximum velocity yet.
   */
  template <typename Particle>
  void computeAdmissibleTimeStepSizeFromGlobalMeshSizeAndMaximumVelocity( double cflFactor, double initialTimeStepSize );
}


#include "TimeStepping.cpph"


