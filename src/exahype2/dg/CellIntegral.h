// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/CellData.h"
#include "exahype2/dg/DGUtils.h"
#include "exahype2/dg/Functors.h"
#include "peano4/Globals.h"
#include "peano4/Loop.h"
#include "peano4/grid/GridControlEvent.h"
#include "tarch/la/Vector.h"

namespace exahype2::dg {
  void cellIntegral_patchwise_in_situ_GaussLegendre_functors(
    ::exahype2::CellData<double, double>& cellData,
    const int                             order,
    const int                             unknowns,
    const int                             auxiliaryVariables,
    Flux                                  flux,
    NonConservativeProduct                nonconservativeProduct,
    Source                                source,
    PointSources                          pointSources,
    const double* NOALIAS                 QuadratureNodes1D,
    const double* NOALIAS                 MassMatrixDiagonal1D,
    const double* NOALIAS                 StiffnessMatrix1D,
    const double* NOALIAS                 DerivativeOperator1D,
    bool                                  evaluateFlux,
    bool                                  evaluateNonconservativeProduct,
    bool                                  evaluateSource,
    bool                                  evaluatePointSources
  );

  template <typename Solver, int order, int unknowns, int auxiliaryVariables>
  void cellIntegral_patchwise_in_situ_GaussLegendre(
    ::exahype2::CellData<double, double>& cellData,
    bool                                  evaluateFlux,
    bool                                  evaluateNonconservativeProduct,
    bool                                  evaluateSource,
    bool                                  evaluatePointSources
  );

  void multiplyWithInvertedMassMatrix_GaussLegendre(
    ::exahype2::CellData<double, double>& cellData,
    const int                             order,
    const int                             unknowns,
    const int                             auxiliaryVariables,
    const double* NOALIAS                 MassMatrixDiagonal1D
  );

  double reduceMaxScaledEigenvalue_patchwise_functors(
    ::exahype2::CellData<double, double>& cellData,
    const int                             order,
    const int                             unknowns,
    const int                             auxiliaryVariables,
    MaxEigenvalue                         maxEigenvalue,
    const double* NOALIAS                 QuadratureNodes1D
  );

  namespace internal {
    void copySolution(
      const double* NOALIAS                     cellQin,
      const int                                 order,
      const int                                 unknowns,
      const int                                 auxiliaryVariables,
      const tarch::la::Vector<DIMENSIONS, int>& node,
      double* NOALIAS                           cellQout
    );
  } // namespace internal
} // namespace exahype2::dg

#include "exahype2/dg/CellIntegral.cpph"
