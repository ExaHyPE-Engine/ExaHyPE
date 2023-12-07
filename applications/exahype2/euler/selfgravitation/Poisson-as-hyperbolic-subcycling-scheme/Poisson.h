#pragma once

#include "AbstractPoisson.h"
#include "tarch/logging/Log.h"

namespace applications::exahype2::euler::selfgravitation {
  class Poisson;
} // namespace applications::exahype2::euler::selfgravitation


class applications::exahype2::euler::selfgravitation::Poisson: public AbstractPoisson {
private:
  static tarch::logging::Log _log;

  static tarch::multicore::BooleanSemaphore _maxGradientDeltaSemaphore;

  double _currentMaxGradientDelta;
  double _previousMaxGradientDelta;

public:
  Poisson();

  // overwritten from AbstractSolver once again
  void startTimeStep(
    double globalMinTimeStamp, double globalMaxTimeStamp, double globalMinTimeStepSize, double globalMaxTimeStepSize
  ) override;

  void reportGradientDelta(double delta);

  /**
   * overwritten
   */
  double getAdmissibleTimeStepSize() const override;


  void initialCondition(
    double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& volumeCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    bool                                         gridIsConstructed
  ) override;


  virtual void boundaryConditions(
    const double* __restrict__ Qinside, // Qinside[4+1]
    double* __restrict__ Qoutside,      // Qoutside[4+1]
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t,
    int                                          normal
  ) override;


public:
  void sourceTerm(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& volumeCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t,
    double                                       dt,
    double* __restrict__ S
  ) override;


  /**
   * Determine max eigenvalue over Jacobian in a given point with solution values
   * (states) Q. All parameters are in.
   *
   * @return Max eigenvalue. Result has to be positive, so we are actually speaking
   *   about the maximum absolute eigenvalue.
   */
  virtual double maxEigenvalue(
    const double* __restrict__ Q, // Q[4+1],
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal
  ) override;


  virtual void flux(
    const double* __restrict__ Q, // Q[4+1],
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double* __restrict__ F // F[4]
  ) override;
};
