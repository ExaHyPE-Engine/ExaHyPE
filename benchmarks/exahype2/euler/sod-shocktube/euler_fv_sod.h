#ifndef _benchmarks_exahype2_euler_sod_shocktube_euler_fv_sod_H_
#define _benchmarks_exahype2_euler_sod_shocktube_euler_fv_sod_H_
#include "Abstracteuler_sod.h"
#include "tarch/logging/Log.h"


namespace bechmarks {namespace exahype2 {namespace euler {namespace sod_shocktube {
  class euler_fv_sod;
}}}}


class benchmarks::exahype2::euler::sod_shocktube::euler_fv_sod: public benchmarks::exahype2::euler::sod_shocktube::Abstracteuler_fv_sod {
  private:
    static tarch::logging::Log   _log;

  public:
     
    void initialCondition(
      double * __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  volumeCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      bool                                         gridIsConstructred
    ) override;

    virtual void boundaryConditions(
      const double * __restrict__ Qinside, // Qinside[4+0]
      double * __restrict__ Qoutside, // Qoutside[4+0]
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      int                                          normal
    )  override;

  public:
   
    /**
     * Determine max eigenvalue over Jacobian in a given point with solution values
     * (states) Q. All parameters are in.
     *
     * @return Max eigenvalue. Result has to be positive, so we are actually speaking
     *   about the maximum absolute eigenvalue.
     */
    virtual double maxEigenvalue(
      const double * __restrict__ Q, // Q[4+0],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal
    ) override;

    virtual void flux(
      const double * __restrict__ Q, // Q[4+0],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double * __restrict__ F // F[4]
    ) override;
    
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod double maxEigenvalue(
      const double * __restrict__ Q, // Q[4+0],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      Offloadable
    );
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif

    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod void flux(
      const double * __restrict__ Q, // Q[4+0],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double * __restrict__                        F, // F[4]
      Offloadable
    );
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif        
};
#endif