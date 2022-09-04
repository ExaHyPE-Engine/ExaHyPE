#ifndef __BENCHMARKS_EXAHYPE2_EULER_EULERFVRUSANOV_H__
#define __BENCHMARKS_EXAHYPE2_EULER_EULERFVRUSANOV_H__

#include "../kernel-benchmarks/AbstractEulerFVRusanov.h"
#include "tarch/logging/Log.h"

#include "EulerKernels.h"

namespace benchmarks { namespace exahype2 { namespace euler {
  class EulerFVRusanov;
}}}

class benchmarks::exahype2::euler::EulerFVRusanov: public benchmarks::exahype2::euler::AbstractEulerFVRusanov {
  private:
    static tarch::logging::Log _log;

  public:
    void initialCondition(
      double * __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  volumeCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      bool                                         gridIsConstructred
    ) override;

    virtual void boundaryConditions(
      const double * __restrict__ Qinside, // Qinside[5+0]
      double * __restrict__ Qoutside, // Qoutside[5+0]
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      int                                          normal
    ) override;

    /**
     * Determine max eigenvalue over Jacobian in a given point with solution values
     * (states) Q. All parameters are in.
     *
     * @return Max eigenvalue. Result has to be positive, so we are actually speaking
     *   about the maximum absolute eigenvalue.
     */
    virtual double maxEigenvalue(
      const double * __restrict__ Q, // Q[5+0],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal
    ) override;

    virtual void flux(
      const double * __restrict__ Q, // Q[5+0],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double * __restrict__ F // F[5]
    ) override;

    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod double maxEigenvalue(
      const double * __restrict__ Q, // Q[5+0],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      Offloadable
    ) {
      return ::applications::exahype2::euler::maxEigenvalueOnGPU(
        Q,
        faceCentre, volumeH, t, dt, normal,
        NumberOfUnknowns, NumberOfAuxiliaryVariables
      );
    }
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif

    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod void flux(
      const double * __restrict__ Q, // Q[5+0],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double * __restrict__                        F, // F[5]
      Offloadable
    ) {
      ::applications::exahype2::euler::fluxOnGPU(
        Q,
        faceCentre, volumeH, t, dt, normal,
        NumberOfUnknowns, NumberOfAuxiliaryVariables,
        F
      );
    }
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
};

#endif
