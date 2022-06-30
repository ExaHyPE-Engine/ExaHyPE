// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_DG_RUSANOV_RIEMANN_H_
#define _EXAHYPE2_DG_RUSANOV_RIEMANN_H_


#include <vector>

#include "../DGUtils.h"
#include "../Functors.h"

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

      namespace internal {
/*
        template <class QInEnumeratorType, class QMaxEigenvalueEnumeratorType>
        static void computeMaxEigenvalue_LoopBody(
          const double* __restrict__                   QIn,
          QInEnumeratorType                            QInEnumerator,
          exahype2::fv::rusanov::MaxEigenvalue         maxEigenvalue,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double* __restrict__                         QMaxEigenvalue,
          QMaxEigenvalueEnumeratorType                 QMaxEigenvalueEnumerator
        )  InlineMethod;
*/
      }
    }
  }
}

#endif
