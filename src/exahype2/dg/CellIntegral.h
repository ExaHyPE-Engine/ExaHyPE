// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "DGUtils.h"
#include "Functors.h"

#include "exahype2/CellData.h"
#include "peano4/Globals.h"
#include "peano4/Loop.h"
#include "peano4/grid/GridControlEvent.h"
#include "tarch/la/Vector.h"
#include "tarch/multicore/multicore.h"

namespace exahype2 {
  namespace dg {
    /**
     * Volumetric kernel for Gauss-Legendre shape functions using functors
     */
    void cellIntegral_patchwise_in_situ_GaussLegendre_functors(
      ::exahype2::CellData<double, double>& cellData,
      const int                             order,
      const int                             unknowns,
      const int                             auxiliaryVariables,
      Flux                                  flux,
      NonConservativeProduct                nonconservativeProduct,
      Source                                source,
      PointSources                          pointSources,
      const double* NOALIAS                 QuadratureNodes1d,
      const double* NOALIAS                 MassMatrixDiagonal1d,
      const double* NOALIAS                 StiffnessMatrix1d,
      const double* NOALIAS                 DerivativeOperator1d,
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

    /**
     * Final step of a DG algorithm step
     */
    void multiplyWithInvertedMassMatrix_GaussLegendre(
      ::exahype2::CellData<double, double>& cellData,
      const int                             order,
      const int                             unknowns,
      const int                             auxiliaryVariables,
      const double* NOALIAS                 MassMatrixDiagonal1d
    );

    /**
     * Compute the maximum scaled eigenvalue over a sequence of cells
     */
    double reduceMaxScaledEigenvalue_patchwise_functors(
      ::exahype2::CellData<double, double>& cellData,
      const int                             order,
      const int                             unknowns,
      const int                             auxiliaryVariables,
      MaxEigenvalue                         maxEigenvalue,
      const double* NOALIAS                 QuadratureNodes1d
    );

    namespace internal {
      /**
       * Copy solution over for one node
       */
      void copySolution(
        const double* NOALIAS                     cellQin,
        const int                                 order,
        const int                                 unknowns,
        const int                                 auxiliaryVariables,
        const tarch::la::Vector<DIMENSIONS, int>& node,
        double* NOALIAS                           cellQout
      );
    } // namespace internal
  } // namespace dg
} // namespace exahype2

#include "CellIntegral.cpph"
