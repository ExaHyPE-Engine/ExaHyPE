// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/CellData.h"
#include "exahype2/dg/DGUtils.h"
#include "exahype2/dg/Functors.h"
#include "peano4/Loop.h"
#include "peano4/grid/GridControlEvent.h"
#include "tarch/la/Vector.h"

namespace exahype2::dg {
  void cellIntegralPatchwiseInSituGaussLegendreFunctors(
    ::exahype2::CellData<double, double>& cellData,
    int                                   order,
    int                                   unknowns,
    int                                   auxiliaryVariables,
    Flux                                  flux,
    NonConservativeProduct                nonconservativeProduct,
    Source                                source,
    const PointSources&                   pointSources,
    const double* NOALIAS                 quadratureNodes,
    const double* NOALIAS                 massMatrixDiagonal,
    const double* NOALIAS                 stiffnessMatrix,
    const double* NOALIAS                 derivativeOperator,
    bool                                  evaluateFlux,
    bool                                  evaluateNonconservativeProduct,
    bool                                  evaluateSource,
    bool                                  evaluatePointSources
  );

  template <typename Solver, int order, int unknowns, int auxiliaryVariables>
  void cellIntegralPatchwiseInSituGaussLegendre(
    ::exahype2::CellData<double, double>& cellData,
    bool                                  evaluateFlux,
    bool                                  evaluateNonconservativeProduct,
    bool                                  evaluateSource,
    bool                                  evaluatePointSources
  );

  void multiplyWithInvertedMassMatrixGaussLegendre(
    ::exahype2::CellData<double, double>& cellData,
    int                                   order,
    int                                   unknowns,
    int                                   auxiliaryVariables,
    const double* NOALIAS                 massMatrixDiagonal1D
  );

  double reduceMaxScaledEigenvaluePatchwiseFunctors(
    ::exahype2::CellData<double, double>& cellData,
    int                                   order,
    int                                   unknowns,
    int                                   auxiliaryVariables,
    MaxEigenvalue                         maxEigenvalue,
    const double* NOALIAS                 quadratureNodes1D
  );

  void copySolution(
    const double* NOALIAS                     cellQin,
    int                                       order,
    int                                       unknowns,
    int                                       auxiliaryVariables,
    const tarch::la::Vector<DIMENSIONS, int>& node,
    double* NOALIAS                           cellQout
  );
} // namespace exahype2::dg

#include "exahype2/dg/CellIntegral.cpph"
