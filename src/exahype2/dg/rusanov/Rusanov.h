// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_DG_RUSANOV_RUSANOV_H_
#define _EXAHYPE2_DG_RUSANOV_RUSANOV_H_


#include <vector>

#include "../DGUtils.h"
#include "../Functors.h"
#include "../CellIntegral.h"

#include "tarch/la/Vector.h"
#include "tarch/multicore/multicore.h"

#include "peano4/grid/GridControlEvent.h"
#include "peano4/utils/Globals.h"


namespace exahype2 {
  namespace dg {
    namespace rusanov {
      /**
       * Solve Riemann problem on face
       *
       * This routine solves the Riemann problem on one face through a Rusanov
       * solver. It is the simplest version of a Riemann solver that you can
       * think of:
       *
       * -
       * -
       * -
       *
       * The implementation is purely point-wise, i.e. we run over the quadrature
       * points and use Rusanov per point. It is not clear if this is reasonable
       * in all cases. It might be better for example to determine the max
       * eigenvalue per face once and then update all quadrature point subject to
       * this one eigenvalue.
       *
       * ## Implementation details
       *
       * The DoFs per face are ordered in standard Peano order:
       *
       *         2 3
       *         0 1
       *   2 3 [ o o ] 2 3
       *   0 1 [ o o ] 0 1
       *         2 3
       *         0 1
       *
       * The lrEnumerator is used for quantities that we evaluate right and left of
       * the face. The faceEnumerator is for data that we evalutate directly on the
       * face.
       */
      void solveRiemannProblem_pointwise_in_situ(
        ::exahype2::dg::Flux                         flux,
        ::exahype2::dg::NonConservativeProduct       ncp,
        ::exahype2::dg::MaxEigenvalue                maxEigenvalue,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  cellSize,
        double                                       t,
        double                                       dt,
        int                                          order,
        int                                          unknowns,
        int                                          auxiliaryVariables,
        int                                          faceNumber,
        const double* __restrict__                   quadraturePoints,
        bool                                         useFlux,
        bool                                         useNcp,
        const double* __restrict__                   projectedValues,
        double* __restrict__                         solution
      );


      /**
       * Delegate to generic implementation in parent directory.
       *
       * static keyword required to avoid multiple definition linker errors.
       */
      static void cellIntegral_patchwise_GaussLegendre_functors(
        ::exahype2::CellData&                          cellData,
        const int                                      order,
        const int                                      unknowns,
        const int                                      auxiliaryVariables,
        Flux                                           flux,
        NonConservativeProduct                         nonConservativeProduct,
        Source                                         source,
        PointSources                                   pointSources,
        const double* __restrict__                     QuadratureNodes1d,
        const double* __restrict__                     MassMatrixDiagonal1d,
        const double* __restrict__                     StiffnessMatrix1d,
        const double* __restrict__                     DerivativeOperator1d,
        bool                                           evaluateFlux,
        bool                                           evaluateNonconservativeProduct,
        bool                                           evaluateSource,
        bool                                           evaluatePointSources
      ) {
        ::exahype2::dg::cellIntegral_patchwise_GaussLegendre_functors(
            cellData,
            order, unknowns, auxiliaryVariables,
            flux,  nonConservativeProduct,  source,  pointSources,
            QuadratureNodes1d,
            MassMatrixDiagonal1d,
            StiffnessMatrix1d,
            DerivativeOperator1d,
            evaluateFlux,  evaluateNonconservativeProduct,  evaluateSource,  evaluatePointSources
          );
      }


      /**
       * Delegate to generic implementation.
       *
       * static keyword required to avoid multiple definition linker errors.
       */
      static void multiplyWithInvertedMassMatrix_GaussLegendre(
        ::exahype2::CellData&                          cellData,
        const int                                      order,
        const int                                      unknowns,
        const int                                      auxiliaryVariables,
        const double* __restrict__                     MassMatrixDiagonal1d
      ) {
        ::exahype2::dg::multiplyWithInvertedMassMatrix_GaussLegendre(
          cellData,
          order, unknowns, auxiliaryVariables, MassMatrixDiagonal1d
        );
      }


      /**
       * Delegate to generic implementation.
       *
       * static keyword required to avoid multiple definition linker errors.
       */
      static void integrateOverRiemannSolutionsAndAddToVolume_GaussLegendre(
        const double* const __restrict__    faceQLeft,
        const double* const __restrict__    faceQRight,
        const double* const __restrict__    faceQBottom,
        const double* const __restrict__    faceQUp,
        int                                 order,
        int                                 unknowns,
        const int                           auxiliaryVariables,
        const tarch::la::Vector<2,double>&  cellSize,
        const double* const __restrict__    BasisFunctionValuesLeft,
        const double* __restrict__          MassMatrixDiagonal1d,
        double* __restrict__                cellQ
      ) {
        ::exahype2::dg::integrateOverRiemannSolutionsAndAddToVolume_GaussLegendre(
          faceQLeft,  faceQRight,  faceQBottom,  faceQUp,
          order, unknowns, auxiliaryVariables,
          cellSize,
          BasisFunctionValuesLeft,
          MassMatrixDiagonal1d,
          cellQ
        );
      }


      /**
       * Delegate to generic implementation.
       *
       * static keyword required to avoid multiple definition linker errors.
       */
      static void integrateOverRiemannSolutionsAndAddToVolume_GaussLegendre(
        const double* const __restrict__    faceQLeft,
        const double* const __restrict__    faceQRight,
        const double* const __restrict__    faceQBottom,
        const double* const __restrict__    faceQUp,
        const double* const __restrict__    faceQFront,
        const double* const __restrict__    faceQBack,
        int                                 order,
        int                                 unknowns,
        const int                           auxiliaryVariables,
        const tarch::la::Vector<3,double>&  cellSize,
        const double* const __restrict__    BasisFunctionValuesLeft,
        const double* __restrict__          MassMatrixDiagonal1d,
        double* __restrict__                cellQ
      ) {
        ::exahype2::dg::integrateOverRiemannSolutionsAndAddToVolume_GaussLegendre(
          faceQLeft,  faceQRight,  faceQBottom,  faceQUp, faceQFront, faceQBack,
          order, unknowns, auxiliaryVariables,
          cellSize,
          BasisFunctionValuesLeft,
          MassMatrixDiagonal1d,
          cellQ
        );
      }
    }
  }
}

#endif
