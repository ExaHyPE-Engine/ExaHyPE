// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/RefinementCommand.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

namespace examples::navierstokes {
  template <class Solver, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD double getMaxEigenvalue(
    const Precision* NOALIAS                     Q,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    int                                          normal,
    const typename Solver::RuntimeParameters&    parameters
  );

  template <class Solver, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD void computeFlux(
    const Precision* NOALIAS                  Q,
    const Precision* const NOALIAS            deltaQ[DIMENSIONS],
    int                                       normal,
    const typename Solver::RuntimeParameters& parameters,
    Precision* NOALIAS                        F
  );

  template <class Solver, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD double computeRusanovFlux(
    const Precision* NOALIAS                     QL,
    const Precision* NOALIAS                     QR,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    int                                          normal,
    const typename Solver::RuntimeParameters&    parameters,
    Precision* NOALIAS                           FL,
    Precision* NOALIAS                           FR
  );

  template <class Solver, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD ::exahype2::RefinementCommand getRefinementCommand(
    const Precision* NOALIAS                  Q,
    const typename Solver::RuntimeParameters& parameters
  );

  template <class Solver, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD ::exahype2::RefinementCommand getRefinementCommandOfBubbles(
    const Precision* NOALIAS                  Q,
    const typename Solver::RuntimeParameters& parameters
  );

  template <class Solver>
  INLINE_METHOD GPU_CALLABLE_METHOD constexpr bool hasGravity();

  template <class Solver, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD double getPressure(
    const Precision* NOALIAS                  Q,
    const typename Solver::RuntimeParameters& parameters
  );

  template <class Solver, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD double getEnergy(
    double                                    pressure,
    double                                    rho,
    const Precision* NOALIAS                  j,
    double                                    height,
    const typename Solver::RuntimeParameters& parameters
  );

  template <class Solver, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD double getConvectiveWaveSpeed(
    const Precision* NOALIAS                  Q,
    int                                       normal,
    const typename Solver::RuntimeParameters& parameters
  );

  template <class Solver, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD double getDiffusiveWaveSpeed(
    const Precision* NOALIAS                  Q,
    const typename Solver::RuntimeParameters& parameters
  );

  template <class Solver, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD void computeGravitySource(
    const Precision* NOALIAS                  Q,
    const typename Solver::RuntimeParameters& parameters,
    Precision* NOALIAS                        S
  );

  template <class Solver, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD void writeWallState(
    const Precision* NOALIAS                     Qinside,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    int                                          normal,
    const typename Solver::RuntimeParameters&    parameters,
    Precision* NOALIAS                           Qoutside
  );

  template <class Solver, typename Precision>
  INLINE_METHOD void writeAtmosphere(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    double                                       perturbation,
    const typename Solver::RuntimeParameters&    parameters,
    Precision* NOALIAS                           Q
  );

  template <class Solver>
  INLINE_METHOD GPU_CALLABLE_METHOD double getHydrostaticPressure(
    double                                    height,
    const typename Solver::RuntimeParameters& parameters
  );

  template <class Solver>
  INLINE_METHOD GPU_CALLABLE_METHOD double getTemperature(
    double                                    potentialTemperature,
    double                                    pressure,
    const typename Solver::RuntimeParameters& parameters
  );
} // namespace examples::navierstokes

#include "examples/navier-stokes/NavierStokes.cpph"
