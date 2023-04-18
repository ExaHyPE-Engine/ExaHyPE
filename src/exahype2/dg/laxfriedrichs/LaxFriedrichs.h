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
    namespace laxfriedrichs {
      /**
       * Solve Riemann problem on face
       *
       * This routine solves the Riemann problem on one face employing the
       * Lax-Friedrichs flux:
       *
       * @f$ F(Q) \approx \frac{1}{2}\Big( F(Q^-) + F(Q^+) \Big) - \frac{h}{2 \Delta T} \Big( Q^+ - Q^- \Big) @f$
       *
       * where the h is the size of the adjacent cells.
       *
       * Please consult the Rusanov Riemann solver as well. Rusanov is basically
       * a Lax-Friedrichs flux, where the damping is controlled by the maximum
       * eigenvalue and thus adopts to the problem characteristics.
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
       *
       *
       * ## Sign discussion
       *
       * We rely on the formulation as discussed on the page Discontinuous Galerkin.
       * See dg.h or the doxygen html pages for details. I also recommend to read
       * through the documentation of cellIntegral_patchwise_in_situ_GaussLegendre_functors()
       * before we dive into the discussion. After reading the statements there,
       * the following rationale should make sense and can be compared to the
       * volumetric statements:
       *
       * - There is no source term.
       * - We have an artificial damping parameter depending on the maximum
       *   eigenvalue. This term enters the Rusanov equation with a minus.
       *   However, it is multiplied with the normal and then we finally have
       *   to bring is to the right-hand side of the equation where the source
       *   term can be found. Different to other solvers, this routine solves
       *   the Riemann problem for the left and right adjacent cell in one go.
       *   The flux always is aligned with the coordinate axis.
       *   So the solution enters the left cell with a minus from damping, and a
       *   minus from bringing it to the right-hand side. So for the left side,
       *   we get a plus. For the right side, we get an additional minus, as we
       *   have an inflow in the opposite direction to the right adjacent
       *   cell's normal.
       * - The flux term discussion follows the discussion of the damping.
       *   Hoewer, the flux term does not have a minus in the original
       *   formulation. It is a simple average. Therefore the signs are exactly
       *   swapped compared to the eigenvalues.
       * - The ncp is nasty. Please consult the generic DG documentation and
       *   maybe also the write-up of the Finite Volume discretisation. Anyway,
       *   it is important to note that the ncp yields non-symmetric terms,
       *   i.e. the flow into the left cell is minus the flow into the right
       *   cell. However, these two terms are still subject to the
       *   multiplication with the normal and we thus have the same ncp
       *   contribution to both left and right result.
       *
       *
       */
      void solveRiemannProblem_pointwise_in_situ(
        ::exahype2::dg::Flux                         flux,
        ::exahype2::dg::NonConservativeProduct       ncp,
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

