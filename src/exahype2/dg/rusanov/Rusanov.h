// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/CellIntegral.h"
#include "exahype2/dg/Functors.h"
#include "peano4/Globals.h"
#include "tarch/la/Vector.h"

namespace exahype2::dg::rusanov {
  void solveRiemannProblem_pointwise_in_situ(
    ::exahype2::dg::Flux                         flux,
    ::exahype2::dg::NonConservativeProduct       nonConservativeProduct,
    ::exahype2::dg::MaxEigenvalue                maxEigenvalue,
    const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       t,
    double                                       dt,
    int                                          order,
    int                                          unknowns,
    int                                          auxiliaryVariables,
    int                                          faceNumber,
    const double* NOALIAS                        quadraturePoints,
    bool                                         evaluateFlux,
    bool                                         evaluateNonconservativeProduct,
    const double* NOALIAS                        projectedValues,
    double* NOALIAS                              solution
  );

  void solveRiemannProblem_pointwise_in_situ_with_gradient_projection(
    ::exahype2::dg::Flux                         flux,
    ::exahype2::dg::NonConservativeProduct       nonConservativeProduct,
    ::exahype2::dg::MaxEigenvalue                maxEigenvalue,
    const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       t,
    double                                       dt,
    int                                          order,
    int                                          unknowns,
    int                                          auxiliaryVariables,
    int                                          faceNumber,
    const double* NOALIAS                        quadraturePoints,
    bool                                         evaluateFlux,
    bool                                         evaluateNonconservativeProduct,
    const double* NOALIAS                        projectedValues,
    double* NOALIAS                              solution
  );

  template <typename Solver, int order, int unknowns, int auxiliaryVariables>
  void cellIntegral_patchwise_in_situ_GaussLegendre(
    ::exahype2::CellData<double, double>& cellData,
    bool                                  evaluateFlux,
    bool                                  evaluateNonconservativeProduct,
    bool                                  evaluateSource,
    bool                                  evaluatePointSources
  ) {
    ::exahype2::dg::cellIntegral_patchwise_in_situ_GaussLegendre<Solver, order, unknowns, auxiliaryVariables>(
      cellData,
      evaluateFlux,
      evaluateNonconservativeProduct,
      evaluateSource,
      evaluatePointSources
    );
  }
} // namespace exahype2::dg::rusanov
