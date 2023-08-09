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
     *
     * @param cflFactor            The constant withint the CFL condition.
     * @param initialTimeStepSize  This value is used if we don't know the mesh
     *   size or maximum velocity yet.
     * @param maxRelativeGrowth    This is the relative max growth of the time
     *   step size per time step. You can set it to max of double if you want
     *   the time step size to jump immediately if it can.
     */
    template <typename Particle>
    void computeAdmissibleTimeStepSizeFromGlobalMeshSizeAndMaximumVelocity(
      double cflFactor, double initialTimeStepSize, double maxRelativeGrowth = 0.1
    );


    //    void computeAdmissibleTimeStepSizeFromGlobalMeshSizeAndMaximumVelocity( double cflFactor, double
    //    initialTimeStepSize );


    /**
     * Determines the maximum global time step size dt allowed by the CLF condition in
     * SPH simulations.
     * The operation overwrites dt only if adjustTimeStepSize is true.
     */
    // @TODO implement the CFL time step for SPH. IT depends not on V but on the signal velocity.
    // At the moment we use this function only to keep dt=constant

    template <typename Particle>
    void computeCFLTimeStepSizeSPH();


  } // namespace timestepping
} // namespace swift2


#include "GlobalTimeStepping.cpph"
