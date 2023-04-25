// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <vector>

#include "../DGUtils.h"
#include "../Functors.h"

/**
 * For the generic kernels that I use here most of the time
 */
#include "../CellIntegral.h"
#include "../Riemann.h"

#include "tarch/la/Vector.h"
#include "tarch/multicore/multicore.h"

#include "peano4/grid/GridControlEvent.h"
#include "peano4/utils/Globals.h"

#if defined(GPUOffloadingOMP)
#include "exahype2/dg/rusanov/omp/Rusanov.h"
#endif


namespace exahype2 {
  namespace dg {
    namespace rusanov {
      /**
       * Solve Riemann problem on face
       *
       * This routine solves the Riemann problem on one face through a Rusanov
       * solver. It takes the average of the flux left and right, reduces it by
       * the jump subject to a scaling with the maximum eigenvalue, and finally
       * adds/substracts the ncp terms.
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
       * The lrEnumerator is used for quantities that we evaluate right and
       * left of the face. The faceEnumerator is for data that we evaluate
       * directly on the face.
       *
       * ## Linearity in h
       *
       * The ncp - as so often - deserves special attention: As we have a
       * non-conservative term here, we don't write the same flux to the left
       * and the right face. Indeed, they carry exactly switched signs. We
       * furthermore note that the original PDE formulation speaks of a
       * term @f$ B(Q) \nabla B @f$ whereas the Finite Volume formulation's
       * Rusanov solver uses a @f$ 0.5 B( 0.5(Q^++Q^-) ) (Q^+-Q^-) @f$ term.
       * Therefore, we need an additional factor of 0.5. However, we do not
       * need any additional calibration, as we simply pass in the Q
       * differences into the DG ncp term which expects the gradient. Whatever
       * comes out of it, should be by a factor of h too big already, i.e.
       * have the right cardinality of the FV expression.
       *
       * In theory, it might be that B is not linear in the vector, i.e. not
       * linear in an h scaling. However, we always can assume that the B that
       * is passed in here is a linearisation along the face. Therefore, we can
       * assume that the scaling is by definition fine with our observation.
       *
       *
       * ## Sign discussion
       *
       * We rely on the formulation as discussed on the page Discontinuous Galerkin.
       * See dg.h or the doxygen html pages for details. I also recommend to read
       * through the documentation of cellIntegral_patchwise_in_situ_GaussLegendre_functors()
       * before we dive into the discussion.
       *
       * All the comments below assume that both the left and the right side of
       * the flux compute the flux in the normal direction. It is then the
       * backpropagation of the Riemann solution which takes into the account
       * that the ``right'' faces denote an outflow, while the ``left'' faces
       * encode an inflow.
       *
       * @see integrateOverRiemannSolutionsAndAddToVolume_GaussLegendre()
       *
       * Bringing all these facts together, we conclude that we end up with a
       * 1:1 translation of Finite Volumes into the DG world.
       *
       * @see exahype2/fv/rusanov/LoopBody.cpph for further information.
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
       *
       * @see solveRiemannProblem_pointwise_in_situ()
       */
      void solveRiemannProblem_pointwise_in_situ_with_gradient_projection(
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
      static void cellIntegral_patchwise_in_situ_GaussLegendre_functors(
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
        ::exahype2::dg::cellIntegral_patchwise_in_situ_GaussLegendre_functors(
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


      template <
        typename Solver,
        int      order,
        int      unknowns,
        int      auxiliaryVariables
      >
      void cellIntegral_patchwise_in_situ_GaussLegendre(
        ::exahype2::CellData&                          cellData,
        bool                                           evaluateFlux,
        bool                                           evaluateNonconservativeProduct,
        bool                                           evaluateSource,
        bool                                           evaluatePointSources
      ) {
        ::exahype2::dg::cellIntegral_patchwise_in_situ_GaussLegendre<Solver,order,unknowns,auxiliaryVariables>(
          cellData,
          evaluateFlux,
          evaluateNonconservativeProduct,
          evaluateSource,
          evaluatePointSources
        );
      }
    }
  }
}

