// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


namespace swift2 {
  namespace timestepping {
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
     *
     * The operation overwrites dt.
     */
    template <typename Particle>
    void computeAdmissibleTimeStepSizeFromGlobalMeshSizeAndMaximumVelocity( double cflFactor, double initialTimeStepSize );
  }
}


#include "GlobalTimeStepping.cpph"


