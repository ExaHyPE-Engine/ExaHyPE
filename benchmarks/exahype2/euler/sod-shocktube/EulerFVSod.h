#pragma once

#include "AbstractEulerFVSod.h"
#include "tarch/logging/Log.h"

namespace benchmarks {namespace exahype2 {namespace euler {namespace sod_shocktube {
  class EulerFVSod;
}}}}

class benchmarks::exahype2::euler::sod_shocktube::EulerFVSod: public benchmarks::exahype2::euler::sod_shocktube::AbstractEulerFVSod {
  private:
    static tarch::logging::Log   _log;

  public: 
    void initialCondition(
      double * __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  volumeCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      bool                                         gridIsConstructed
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
};
