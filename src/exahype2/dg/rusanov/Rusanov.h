// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/Functors.h"
#include "tarch/la/Vector.h"

namespace exahype2::dg::rusanov {
  void solveRiemannProblemPointwiseInSitu(
    const ::exahype2::dg::Flux&                   flux,
    const ::exahype2::dg::NonConservativeProduct& nonConservativeProduct,
    const ::exahype2::dg::MaxEigenvalue&          maxEigenvalue,
    const tarch::la::Vector<DIMENSIONS, double>&  faceCentre,
    const tarch::la::Vector<DIMENSIONS, double>&  cellSize,
    double                                        t,
    double                                        dt,
    int                                           order,
    int                                           unknowns,
    int                                           auxiliaryVariables,
    int                                           faceNumber,
    const double* NOALIAS                         quadraturePoints,
    bool                                          evaluateFlux,
    bool                                          evaluateNonconservativeProduct,
    const double* NOALIAS                         projectedValues,
    double* NOALIAS                               solution
  );

  void solveRiemannProblemPointwiseInSituWithGradientProjection(
    const ::exahype2::dg::Flux&                   flux,
    const ::exahype2::dg::NonConservativeProduct& nonConservativeProduct,
    const ::exahype2::dg::MaxEigenvalue&          maxEigenvalue,
    const tarch::la::Vector<DIMENSIONS, double>&  faceCentre,
    const tarch::la::Vector<DIMENSIONS, double>&  cellSize,
    double                                        t,
    double                                        dt,
    int                                           order,
    int                                           unknowns,
    int                                           auxiliaryVariables,
    int                                           faceNumber,
    const double* NOALIAS                         quadraturePoints,
    bool                                          evaluateFlux,
    bool                                          evaluateNonconservativeProduct,
    const double* NOALIAS                         projectedValues,
    double* NOALIAS                               solution
  );
} // namespace exahype2::dg::rusanov
